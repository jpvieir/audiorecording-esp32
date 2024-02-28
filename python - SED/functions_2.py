import os
import io
import shutil
import requests
import struct
import wave

sample_rate = 16000; channels = 1
API_KEY = ""

print("test")

def retrive_pcm_filenames_from_folder(folder_id, output_directory):
    
    url = f"https://www.googleapis.com/drive/v3/files?q='{folder_id}'+in+parents+and+name contains '.pcm'&key={API_KEY}"
    response = requests.get(url)
    data = response.json()

    if "files" not in data:
        print("No .pcm files found in the folder.")
        return

    file_ids = []; filenames = []; 
    for file_info in data["files"]:
        file_ids.append(file_info["id"])
        filenames.append(file_info["name"])
    
    return file_ids, filenames
        
print("test 2") 

def download_pcm_files_from_folder(file_dict, folder_id, output_directory):
   
    if not file_dict:
        print("The list is empty. No files need to be downloaded.")
        return
    
    for file_id in file_dict:
        file_name = file_dict[file_id]
        download_url = f"https://www.googleapis.com/drive/v3/files/{file_id}?alt=media&key={API_KEY}"
        response = requests.get(download_url)
        if response.status_code == 200:
            output_file_path = output_directory + '/' + file_name
            
            with open(output_file_path, "wb") as f:
                f.write(response.content)

            print(f"Downloaded: {file_name}")
        else:
            print(f"Failed to download: {file_name}")

def add_wav_header(pcm_data, sample_rate, channels):
    num_samples = len(pcm_data) // 2  # Divide by 2 since each sample is 2 bytes (16 bits)
    byte_rate = sample_rate * channels * 2  # Sample rate * channels * bytes per sample
    block_align = channels * 2  # Channels * bytes per sample

    wav_header = (
        b"RIFF"  # ChunkID
        + struct.pack("<I", 36 + len(pcm_data))  # ChunkSize
        + b"WAVE"  # Format
        + b"fmt "  # Subchunk1ID
        + struct.pack("<I", 16)  # Subchunk1Size (PCM format size)
        + struct.pack("<H", 1)  # AudioFormat (PCM = 1)
        + struct.pack("<H", channels)  # NumChannels
        + struct.pack("<I", sample_rate)  # SampleRate
        + struct.pack("<I", byte_rate)  # ByteRate
        + struct.pack("<H", block_align)  # BlockAlign
        + struct.pack("<H", 16)  # BitsPerSample (16-bit PCM)
        + b"data"  # Subchunk2ID
        + struct.pack("<I", len(pcm_data))  # Subchunk2Size (PCM data size)
    )

    return wav_header + pcm_data

def convert_little_endian(file):
    with open(file, 'rb') as f:
        pcm_data = f.read()

    little_endian_data = bytearray(len(pcm_data))
    for i in range(0, len(pcm_data), 2):
        little_endian_data[i] = pcm_data[i + 1]
        little_endian_data[i + 1] = pcm_data[i]

    return add_wav_header(little_endian_data, sample_rate, channels)

def post_audio(file_list):
    
    url = 'http://sed.linse.ufsc.br/process'
    header = {'Content-Type' : 'audio/wav'}
    
    pred = []
    
    for i in file_list:
        data = convert_little_endian('files/' + i)
        response = requests.post(url, data=data, headers=header)
        
        if (response.status_code == 200):
            resp = (response.json())
        
        pred.append((resp['segments'])[0])
    
    return pred

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
