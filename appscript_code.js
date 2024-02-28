// Replace with the desired folder ID in Google Drive where you want to store the files.
const DESTINATION_FOLDER_ID = "";

/*
function doGet(x) {
  // Extract the filename from the query parameters
  var filename = x.parameter.filename;

  PropertiesService.getScriptProperties().setProperty("filename", filename);

  // Return a response to the ESP32 to acknowledge receiving the filename
  var response = ContentService.createTextOutput("Filename received: " + filename);
  response.setMimeType(ContentService.MimeType.TEXT);
  return response;
}
*/

function doGet(e) {
  var filename = e.parameter.filename;
  var fileSize = e.parameter.fileSize;
  var checkFileSize = e.parameter.check_filesize;
  var deleteFile = e.parameter.delete_file;

  // Store the filename in Script Properties
  PropertiesService.getScriptProperties().setProperty("filename", filename);
  PropertiesService.getScriptProperties().setProperty("fileSize", fileSize);


  // Check if the checkFileSize parameter is set to true
  if (checkFileSize && checkFileSize.toLowerCase() === "true") {
    // Get the file ID of the file with the provided filename
    var fileId = getFileIdByName(filename);
    if (fileId) {
      PropertiesService.getScriptProperties().setProperty("fileId", fileId);
      // Get the file size of the file in Google Drive
      var fileSizeOnDrive = getFileSize(fileId);
      

      // Compare the file size from the ESP32 with the file size on Google Drive
      var isFileSizeMatching = (fileSizeOnDrive == fileSize);

      // Return the result as a response
      return ContentService.createTextOutput(isFileSizeMatching.toString());
    } else {
      // File not found in Google Drive, return false
      return ContentService.createTextOutput("false");
      //return ContentService.createTextOutput(fileId);

    }
  } 

  if (deleteFile && deleteFile.toLowerCase() == "true") {

    var fileId = getFileIdByName(filename);
    var result = deleteFileById(fileId);

    var i = 0;

    while (result == "false") {
      i = i+1; 
      result = deleteFileById(fileId);
      if (i>= 3) {
        break; 
      }
    } 
    
    return ContentService.createTextOutput(result);

  }
  else {
    return ContentService.createTextOutput("Request received successfully");
  }
}

function doPost(e) {
 
  var filename = PropertiesService.getScriptProperties().getProperty("filename");
  var fileSize = PropertiesService.getScriptProperties().getProperty("fileSize");
  var contentBlob = e.postData;
  var bytes = contentBlob.getBytes();
  //var filenameHeader = e.parameter.filename;
  //var filename = filenameHeader ? filenameHeader : "default_file_name.pcm"; // Replace with a default filename

  // Save the binary data to Google Drive
  var folder = DriveApp.getFolderById(DESTINATION_FOLDER_ID);
  //var files = folder.getFilesByName(FILE_NAME);
  var files = folder.getFilesByName(filename);

  // Check if the file already exists
  if (files.hasNext()) {
    var file = files.next();
    var fileId = file.getId();
    
    // Fetch the existing file content as a Blob
    var existingData = file.getBlob();

    // Append the new data to the existing file content
    //var newData = convertToLittleEndian(bytes);
    var updatedData = concatUint8Arrays(existingData.getBytes(),bytes);
    //var updatedData = existingData.getBytes().concat(newData);
    //var byteArray = Array.from(updatedData)

    // Use the Advanced Drive Service to update the file content
    Drive.Files.update({}, fileId, Utilities.newBlob(updatedData,"application/octet-stream"));
  } else {
    // If the file doesn't exist, create a new one
    //var newFile = Drive.Files.insert({ title: FILE_NAME, mimeType: "application/octet-stream", parents: [{ id: DESTINATION_FOLDER_ID }] }, Utilities.//newBlob(data));
    //Drive.Files.insert({ title: FILE_NAME, mimeType: "audio/x-pcm", parents: [{ id: DESTINATION_FOLDER_ID }] }, Utilities.newBlob(data));
    //var header = createWavHeader(fileSize-44, 16000, 16);
    //var littleEndianData = convertToLittleEndian(bytes);
    //var newData = header.concat(bytes);
    //var newData = concatUint8Arrays(header, littleEndianData);
    var blob = Utilities.newBlob(bytes, "application/octet-stream", filename);
    var file = folder.createFile(blob);

  }

  // Return a response to the ESP32
  var response = ContentService.createTextOutput("File saved successfully");
  response.setMimeType(ContentService.MimeType.TEXT);
  return response;
}

function getFileIdByName(filename) {
  try {
    var files = DriveApp.getFilesByName(filename);
    if (files.hasNext()) {
      return files.next().getId();
    }
    return null; // Return null if the file is not found
  } catch (error) {
    return null; // Return null if an error occurs
  }
}

function getFileSize(fileId) {
  try {
    var file = DriveApp.getFileById(fileId);
    return file.getSize();
  } catch (error) {
    return -1; // Return -1 to indicate an error
  }
}

function deleteFileById(fileId) {
  try {
    Drive.Files.remove(fileId);
    Logger.log("File with ID '" + fileId + "' was successfully deleted.");
    return "true";
  } catch (error) {
    Logger.log("Error deleting file with ID '" + fileId + "': " + error);
    return "false";
  }
}

function createWavHeader(dataSize, sampleRate, bitDepth) {
  var header = new Uint8Array(44);

  // RIFF chunk descriptor
  writeStringToHeader(header, 0, "RIFF");
  writeUint32ToHeader(header, 4, dataSize + 36);

  // Format chunk
  writeStringToHeader(header, 8, "WAVE");
  writeStringToHeader(header, 12, "fmt ");
  writeUint32ToHeader(header, 16, 16);
  writeUint16ToHeader(header, 20, 1); // Audio Format (1 for PCM)
  writeUint16ToHeader(header, 22, 1); // Number of channels
  writeUint32ToHeader(header, 24, sampleRate);
  writeUint32ToHeader(header, 28, sampleRate * 2); // Byte Rate (Sample Rate * Number of Channels * Bits per Sample / 8)
  writeUint16ToHeader(header, 32, 2); // Block Align (Number of Channels * Bits per Sample / 8)
  writeUint16ToHeader(header, 34, bitDepth); // Bits per Sample

  // Data chunk
  writeStringToHeader(header, 36, "data");
  writeUint32ToHeader(header, 40, dataSize);

  return header;
}

// Helper functions to write data to header
function writeStringToHeader(header, offset, str) {
  for (var i = 0; i < str.length; i++) {
    header[offset + i] = str.charCodeAt(i);
  }
}

function writeUint32ToHeader(header, offset, value) {
  header[offset] = value & 0xFF;
  header[offset + 1] = (value >> 8) & 0xFF;
  header[offset + 2] = (value >> 16) & 0xFF;
  header[offset + 3] = (value >> 24) & 0xFF;
}

function writeUint16ToHeader(header, offset, value) {
  header[offset] = value & 0xFF;
  header[offset + 1] = (value >> 8) & 0xFF;
}
function concatUint8Arrays(array1, array2) {
  var newArray = new Uint8Array(array1.length + array2.length);
  newArray.set(array1, 0);
  newArray.set(array2, array1.length);
  return newArray;
}

function convertToLittleEndian(bigEndianData) {
  var littleEndianData = new Uint8Array(bigEndianData.length);
  for (var i = 0; i < bigEndianData.length; i += 2) {
    littleEndianData[i] = bigEndianData[i + 1];
    littleEndianData[i + 1] = bigEndianData[i];
  }
  return littleEndianData;
}

