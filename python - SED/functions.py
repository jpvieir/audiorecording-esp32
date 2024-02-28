import os
import io
import shutil
import requests
import struct
import wave
from google.auth.transport.requests import Request
from google_auth_oauthlib.flow import InstalledAppFlow
import googleapiclient.discovery
import pickle
import librosa
import numpy as np
import wave


sample_rate = 16000; channels = 1
API_KEY = ""
folder_id = ""
output_directory = ""
SCOPES = ['https://www.googleapis.com/auth/drive.readonly']
TOKEN_PICKLE_FILENAME = "token.pickle"

def authenticate_with_drive_api():
    # Create a flow instance for OAuth 2.0
    flow = InstalledAppFlow.from_client_secrets_file(
        'C:/Users/....../credentials.json'
        , scopes=SCOPES
    )

    if os.path.exists(TOKEN_PICKLE_FILENAME):
        with open(TOKEN_PICKLE_FILENAME, 'rb') as token:
            credentials = pickle.load(token)
    else:
        # Run the OAuth 2.0 authorization flow
        credentials = flow.run_local_server()

        # Save the token for future use
        with open(TOKEN_PICKLE_FILENAME, 'wb') as token:
            pickle.dump(credentials, token)

    drive_service = googleapiclient.discovery.build('drive', 'v3', credentials=credentials)

    return drive_service


def retrive_wav_filenames_from_folder(folder_id):

    url = f"https://www.googleapis.com/drive/v3/files?q='{folder_id}'+in+parents+and+name contains '.wav'&key={API_KEY}"
    response = requests.get(url)
    data = response.json()

    if "files" not in data:
        print("No .wav files found in the folder.")
        return

    file_ids = []; filenames = []; 
    for file_info in data["files"]:
        file_ids.append(file_info["id"])
        filenames.append(file_info["name"])
    
    return file_ids, filenames
        

def download_wav_files_from_folder(file_dict, output_directory, drive_service):
   
    if not file_dict:
        print("There are no new files.")
        return False
    
    for file_id in file_dict:
        file_name = file_dict[file_id]

        request = drive_service.files().get_media(fileId=file_id)

        output_file_path = output_directory + '/' + file_name
        # Save the downloaded content to a file on disk
        with open(output_file_path, "wb") as file:
            downloader = googleapiclient.http.MediaIoBaseDownload(file, request)
            done = False
            while not done:
                status, done = downloader.next_chunk()
                print(f"Downloading file {file_name}. Progress:  {int(status.progress() * 100)}%")     
                print("-----------------------------------------")   

    return True



def post_audio(file_list):

    #if not file_list:
    #    print("No files need to be downloaded.")
    #    return
    
    url = 'http://sed.linse.ufsc.br/process'
    header = {'Content-Type' : 'audio/wav'}
    
    preds = ''

    print(file_list)
    
    for i in file_list:
        path = 'C:/..../python - SED/files/' + i
        #data,sr  = librosa.load(path, sr =16000 , mono=None)
        #print(data.shape)
        print(path)
        file = open(path, "rb")
        data = file.read()
        response = requests.post(url, data= (data), headers=header)


        print("File: " + i)
        preds = preds + "File: " + i + "\n"
        if (response.status_code == 200):
            resp = (response.json())
            print("Predictions: ")
            pred = (resp['segments'])[0]

           
            for k in range(5):
                line = 'Class: ' + pred['classes'][k]['class_name'] + '. Score: ' + str(pred['classes'][k]['score'] * 100)[0:4] + '%.'
                print(line)
                preds = preds + (line + '\n')

        else:
            print("Error: code " + str(response.status_code) + ".")
            return False

        print("-----------------------------------------")
        preds = preds + ("-----------------------------------------\n")

    
    
    return True, preds

def get_filelist_fromDisk():
    file_list = os.listdir(output_directory)

    for i in file_list:
        if i.endswith(".pcm") == False:
            file_list.remove(i)
    
    return file_list

def compare_and_delete(files_to_download, files_in_disk):
    set1 = set(files_to_download); set2 = set(files_in_disk)
    common_items = set1.intersection(set2)

    # Remove common items from both lists
    for item in common_items:
        while item in files_to_download:
            files_to_download.remove(item)
    return files_to_download


def convert_big_endian_to_little_endian(data):
    little_endian_data = bytearray(len(data))
    for i in range(0, len(data), 2):
        little_endian_data[i] = data[i + 1]
        little_endian_data[i + 1] = data[i]

    return little_endian_data