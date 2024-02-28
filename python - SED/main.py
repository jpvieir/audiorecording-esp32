## This code downloads .wav audio files from a Google Drive folder and sends them to SED-LINSE
## SED-LINSE returns 5 predictions of the sound, having a total of 500 classes available. 
## Every {DELAY_TIME} seconds the code checks if there is a new file available to download on the folder; 
## If there are new files, the code downloads them and send them to SED-LINSE. 

from functions import retrive_wav_filenames_from_folder
from functions import get_filelist_fromDisk
from functions import compare_and_delete
from functions import download_wav_files_from_folder
from functions import post_audio
import time 
from functions import authenticate_with_drive_api
from datetime import datetime
import numpy as np 

folder_id = "" ## Google Drive Folder ID 
output_directory = "..../python - SED/files"
DELAY_TIME = 120 ## Delay time, in seconds

drive_service = authenticate_with_drive_api() ## required to authenticate with Drive API 
file_id, filenames = retrive_wav_filenames_from_folder(folder_id)
files_disk = get_filelist_fromDisk()
files_to_download = compare_and_delete(filenames, files_disk) ## check if needs to download new files 
file_dict = {file_id[i]: filenames[i] for 
             wai in range(len(filenames))}
post = download_wav_files_from_folder(file_dict, output_directory, drive_service)

file_list = np.sort(get_filelist_fromDisk())
preds = post_audio(file_list) ## get the predictions 

with open('predictions.txt', 'w') as f:
    f.write(preds) ## save the predictions in a .txt file  

while(True):
  
    # delay 
    time.sleep(DELAY_TIME)

    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    print("Current Time =", current_time)

    file_id, filenames = retrive_wav_filenames_from_folder(folder_id, output_directory)
    files_disk = get_filelist_fromDisk()
    files_to_download = (compare_and_delete(filenames, files_disk))
    if(files_to_download): 
        files_disk = files_disk.extend(files_to_download)
    file_dict = {file_id[i]: files_to_download[i] for i in range(len(files_to_download))}
    post = download_wav_files_from_folder(file_dict, folder_id, output_directory, drive_service)#

    if(post):
        success, preds = post_audio(np.sort(files_to_download))
        if(success):
            with open('predictions.txt', 'a') as f:
                f.write(preds)

    

    print(pred)