#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "stdlib.h"
#include <string.h>
#include "record_mgr.h"


int maximum_Pages = 5;

/*
 * Initializes the Record Manager module.
 * This function initializes the Record Manager module by calling the `initStorageManager` function,
 * which initializes the underlying storage manager.
 *
 * Parameters:
 * - managementData: A pointer to optional management data. This parameter is not used in this function.
 *
 * Returns:
 * - RC_OK: The Record Manager module was initialized successfully.
 */
extern RC initRecordManager (void *managementData) {
    printf("Initializing the record manager ");
    initStorageManager();
    return RC_OK;
}

/*
 * Shuts down the Record Manager module.
 *
 * This function shuts down the Record Manager module.
 *
 * Returns:
 * - RC_OK: The Record Manager module was shut down successfully.
 */
extern RC shutdownRecordManager () {
    //free(RM_managementData);
    printf("Shutting down the record manager\n");
    return RC_OK;
}

/*
 * Creates a new table with the specified name and schema.
 * This function creates a new table with the given name and schema. It creates the underlying
 * page file, stores information about the schema, and initializes the table's metadata.
 *
 * Parameters:
 * - name: The name of the table (corresponds to the name of the page file).
 * - schema: The schema defining the structure of the table.
 *
 * Returns:
 * - RC_OK: The table was created successfully.
 */
extern RC createTable(char *name, Schema *schema) {
    // Check if the schema or name is null
    if (name == NULL || schema == NULL) {
        return RC_INVALID_INPUT;
    }

    // Create the underlying page file, with error handling
    RC createFileRC = createPageFile(name);
    if (createFileRC != RC_OK) {
        return createFileRC;
    }

    SM_FileHandle fileHandle;

    // Attempt to open the page file and check for errors
    RC openFileRC = openPageFile(name, &fileHandle);
    if (openFileRC != RC_OK) {
        return openFileRC;
    }

    // Allocate memory for the Table Information page, with error check
    SM_PageHandle pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);
    if (pageHandle == NULL) {
        closePageFile(&fileHandle);
        return RC_MEMORY_ALLOCATION_FAIL;
    }
    memset(pageHandle, 0, PAGE_SIZE);

    // Initialize offset for writing schema info to the page
    int offset = 0;

// Copy the number of attributes into the page handle
if (PAGE_SIZE < offset + sizeof(int)) {
    free(pageHandle);
    closePageFile(&fileHandle);
    return RC_PAGE_FULL; // Early exit if page is full
}
// Copy the number of attributes to the page handle and adjust the offset
*((int *)(pageHandle + offset)) = schema->numAttr;
offset += sizeof(int);


// Loop through attribute names and store them with length checks
for (int i = 0; i < schema->numAttr; i++) {
    const char *attrName = schema->attrNames[i];
    int lengthName = strlen(attrName) + 1; // Include null terminator

    // Check if there is enough space for the attribute name
    if (offset + lengthName > PAGE_SIZE) {
        free(pageHandle);
        closePageFile(&fileHandle);
        return RC_PAGE_FULL; // Handle insufficient space
    }

    // Copy the attribute name into the page handle
    memcpy(pageHandle + offset, attrName, lengthName);
    offset += lengthName; // Update offset for the next entry
}
    // Store data types with bounds checking
    if (offset + (schema->numAttr * sizeof(DataType)) > PAGE_SIZE) {
        free(pageHandle);
        closePageFile(&fileHandle);
        return RC_PAGE_FULL;
    }
    memcpy(pageHandle + offset, schema->dataTypes, schema->numAttr * sizeof(DataType));
    offset += schema->numAttr * sizeof(DataType);



    // Store attribute lengths with bounds checking
    if (offset + (schema->numAttr * sizeof(int)) > PAGE_SIZE) {
        free(pageHandle);
        closePageFile(&fileHandle);
        return RC_PAGE_FULL;
    }
    memcpy(pageHandle + offset, schema->typeLength, schema->numAttr * sizeof(int));
    offset += schema->numAttr * sizeof(int);

    // Store key attributes with bounds checking
    if (offset + sizeof(int) + (schema->keySize * sizeof(int)) > PAGE_SIZE) {
        free(pageHandle);
        closePageFile(&fileHandle);
        return RC_PAGE_FULL;
    }
   // Copy keySize and update the offset
    int keySize = schema->keySize;
    memcpy(pageHandle + offset, &keySize, sizeof(keySize));
    offset += sizeof(keySize);

    // Copy keyAttrs and update the offset
    int keyAttrsSize = keySize * sizeof(int);
    memcpy(pageHandle + offset, schema->keyAttrs, keyAttrsSize);
    offset += keyAttrsSize;

    // Write the Table Information page to the file, with error handling
    RC writeRC = writeBlock(0, &fileHandle, pageHandle);
    if (writeRC != RC_OK) {
        free(pageHandle);
        closePageFile(&fileHandle);
        return writeRC;
    }

    // Free the allocated memory for pageHandle
    free(pageHandle);

    // Initialize and write the page directory
    int numPages = 1;
    int numPageDP = 1;
    PageDirectoryEntry *pageDirectory = (PageDirectoryEntry *) malloc(sizeof(PageDirectoryEntry));
    if (pageDirectory == NULL) {
        closePageFile(&fileHandle);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    // Initialize the first page directory entry using a struct initializer
PageDirectoryEntry firstPage = {
    .pageID = 0,
    .hasFreeSlot = true,
    .freeSpace = PAGE_SIZE,
    .recordCount = 0
};

// Assign the initialized page entry to the first entry in the page directory
pageDirectory[0] = firstPage;

// Allocate memory for the page handle
pageHandle = malloc(PAGE_SIZE);

    if (pageHandle == NULL) {
        free(pageDirectory);
        closePageFile(&fileHandle);
        return RC_MEMORY_ALLOCATION_FAIL;
    }
    // Clear the memory for the page handle
for (int i = 0; i < PAGE_SIZE; i++) {
    pageHandle[i] = 0;
}

    // Copy the page directory information
    offset = 0;
    memcpy(pageHandle + offset, &numPages, sizeof(numPages));
    offset += sizeof(numPages);

    memcpy(pageHandle + offset, &numPageDP, sizeof(numPageDP));
    offset += sizeof(numPageDP);

    memcpy(pageHandle + offset, pageDirectory, sizeof(PageDirectoryEntry));

    // Write the page directory to the file
writeRC = writeBlock(1, &fileHandle, pageHandle);
    if (writeRC != RC_OK) {
        free(pageDirectory);
        free(pageHandle);
        closePageFile(&fileHandle);
        return writeRC;
    }

    // Clean up resources
    free(pageDirectory);
    free(pageHandle);

    // Close the file handle
    RC closeFileRC = closePageFile(&fileHandle);
    if (closeFileRC != RC_OK) {
        return closeFileRC;
    }

    return RC_OK;
}



extern RC openTable(RM_TableData *rel, char *name) {
    // Allocate memory for RM_TableData structure
    rel->name = name;
    rel->schema = (Schema *)calloc(1, sizeof(Schema));  // Using calloc for zero-initialization
    rel->managementData = calloc(1, sizeof(RM_managementData));  // Using calloc for zero-initialization
    RM_managementData *managementData = (RM_managementData *)(rel->managementData);


// Declare return code variable
RC rc;
switch (rc = openPageFile(name, &managementData->fileHndl)) {
    case RC_OK:
        // Successfully opened the page file, proceed with initialization
        break;
    default:
        // Cleanup and return if there's an error opening the file
        free(rel->schema);
        free(rel->managementData);
        return rc;
}

// Initialize the buffer pool with LRU replacement strategy
rc = initBufferPool(&managementData->bm, name, maximum_Pages, RS_LRU, NULL);
if (rc != RC_OK) {
    return rc;  // Early exit if buffer pool setup fails
}

// Attempt to load the first page (Table Information page)
rc = pinPage(&managementData->bm, &managementData->pageHndlBM, 0);
if (rc != RC_OK) {
    return rc;  // Return if unable to pin the page
}

// Allocate memory for the page contents and zero-initialize it
managementData->memPageSM = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));  // Memory zeroed out

memcpy(managementData->memPageSM, managementData->pageHndlBM.data, PAGE_SIZE);
unpinPage(&managementData->bm, &managementData->pageHndlBM);

// Extract schema information from the Table Information page
char *pageData = (char *)managementData->memPageSM;
int offset = sizeof(int); // Start offset after numAttr
memcpy(&rel->schema->numAttr, pageData, offset);
rel->schema->attrNames = malloc(rel->schema->numAttr * sizeof(char *)); // Allocate memory for attribute names


    // Read attribute names and update the offset
for (int i = 0; i < rel->schema->numAttr; i++) {
    char *attrName = strdup(pageData + offset); // Duplicate the attribute name
    rel->schema->attrNames[i] = attrName;
    offset += strlen(attrName) + 1; // Adjust the offset for the next attribute
}

// Allocate and copy memory for data types
int dataTypeSize = rel->schema->numAttr * sizeof(DataType);
rel->schema->dataTypes = (DataType *)malloc(dataTypeSize);
memcpy(rel->schema->dataTypes, pageData + offset, dataTypeSize);
offset += dataTypeSize; // Update offset

// Allocate and copy memory for type lengths
int typeLengthSize = rel->schema->numAttr * sizeof(int);
rel->schema->typeLength = (int *)malloc(typeLengthSize);
memcpy(rel->schema->typeLength, pageData + offset, typeLengthSize);
offset += typeLengthSize; // Update offset again

// Read and store the key size and key attributes
memcpy(&rel->schema->keySize, pageData + offset, sizeof(int));
offset += sizeof(int); // Update offset after reading key size

int keyAttrSize = rel->schema->keySize * sizeof(int);
rel->schema->keyAttrs = (int *)malloc(keyAttrSize);
memcpy(rel->schema->keyAttrs, pageData + offset, keyAttrSize);
offset += keyAttrSize; // Adjust offset for key attributes

// Clean up previous memory
if (managementData->memPageSM) {
    free(managementData->memPageSM); // Free memory if not null
}

// Pin the second page and load the page data
// Pin the page in the buffer
pinPage(&managementData->bm, &managementData->pageHndlBM, 1);

// Allocate memory for the in-memory page
managementData->memPageSM = (SM_PageHandle)malloc(PAGE_SIZE);
if (managementData->memPageSM == NULL) {
    unpinPage(&managementData->bm, &managementData->pageHndlBM); // Ensure the page is unpinned if malloc fails
    return RC_MEM_ALLOCATION_FAIL; // Handle memory allocation failure
}

// Copy the contents from the buffer to the allocated memory
memcpy(managementData->memPageSM, managementData->pageHndlBM.data, PAGE_SIZE);

// Release the pinned page from the buffer pool
unpinPage(&managementData->bm, &managementData->pageHndlBM);


// Extract the number of pages and directory pages from the in-memory page
int *pageInfo = (int *)managementData->memPageSM;
managementData->numPages = pageInfo[0];
managementData->numPageDP = pageInfo[1];


// Allocate memory for the page directory and copy the data
int numEntries = managementData->numPages - managementData->numPageDP + 1;
int pageDirSize = numEntries * sizeof(PageDirectoryEntry);
managementData->pageDirectory = (PageDirectoryEntry *)malloc(pageDirSize);
memcpy(managementData->pageDirectory, managementData->memPageSM + 2 * sizeof(int), pageDirSize);

// Free the memory used by the in-memory page
free(managementData->memPageSM);
managementData->memPageSM = NULL; // Set the pointer to NULL after freeing


    return RC_OK;
}


/*
 * Closes the specified table.
 * This function closes the specified table, frees memory allocated for schema information,
 * page directory, and management data, and shuts down the buffer pool.
 *
 * Parameters:
 * - rel: Pointer to the RM_TableData structure representing the table.
 *
 * Returns:
 * - RC_OK: The table was closed successfully.
 */
extern RC closeTable(RM_TableData *rel) {
    if (rel == NULL || rel->managementData == NULL || rel->schema == NULL) {
        return RC_OK; // Nothing to close
    }

    RM_managementData *managementData = (RM_managementData *)rel->managementData;

    // Free the schema information
    if (rel->schema->attrNames != NULL) {
    int i = 0; // Initialize the index variable
    while (i < rel->schema->numAttr) {
        free(rel->schema->attrNames[i]);
        i++; // Increment the index
    }
    free(rel->schema->attrNames);
}

free(rel->schema->dataTypes);
free(rel->schema->typeLength);





free(rel->schema->keyAttrs);
free(rel->schema);


    // Free the page directory
    if (managementData->pageDirectory != NULL) {
        free(managementData->pageDirectory);
    }

    // Shutdown the buffer pool
    RC rc = shutdownBufferPool(&managementData->bm);
    if (rc != RC_OK) {
        return rc; // Return error if shutdown fails
    }

    // Close the table file
    rc = closePageFile(&managementData->fileHndl);
    if (rc != RC_OK) {
        return rc; // Return error if close fails
    }

    // Free the management data
    free(rel->managementData);

    return RC_OK;
}



/*
 * Deletes the specified table.
 * This function deletes the underlying page file associated with the specified table.
 *
 * Parameters:
 * - name: The name of the table to delete.
 *
 * Returns:
 * - RC_OK: The table was deleted successfully.
 */
extern RC deleteTable(char *name) {
    if (name == NULL) {
        return RC_INVALID_NAME; // Handle case where the name is NULL
    }

    RC rc = destroyPageFile(name);
    if (rc != RC_OK) {
        return rc; // Return the error code if the deletion fails
    }

    return RC_OK; // Return success if the deletion was successful
}


/*
 * Gets the total number of tuples stored in the table.
 * This function retrieves the total number of tuples stored in the table by summing up
 * the record count from each page directory entry.
 *
 * Parameters:
 * - rel: Pointer to the RM_TableData structure representing the table.
 *
 * Returns:
 * - The total number of tuples stored in the table.
 */

extern int getNumTuples(RM_TableData *rel) {
    // Ensure the relation and management data are valid
    if (rel == NULL) {
        return 0; // No tuples if the relation is NULL
    }

    RM_managementData *managementData = (RM_managementData *) rel->managementData;

    if (managementData == NULL) {
        return 0; // No tuples if management data is NULL
    }

    int totalTuples = 0; // Variable to store the total number of tuples

    // Traverse each page in the directory and count the tuples
   int pageIdx = 0; // Initialize the page index
while (pageIdx < managementData->numPages) {
    PageDirectoryEntry *currentEntry = &managementData->pageDirectory[pageIdx];
    totalTuples += currentEntry->recordCount; // Add record count from current page
    pageIdx++; // Increment the page index
}
    return totalTuples; // Return the final tuple count
}


extern RC insertRecord (RM_TableData *rel, Record *record){
RM_managementData *managementData = (RM_managementData *)rel->managementData;

// Define constants for maximum page directory entries and record size
int recordSize = getRecordSize(rel->schema); int maxEntriesInPD = (PAGE_SIZE - 2 * sizeof(int)) / sizeof(PageDirectoryEntry);


// Determine if a new page directory is required
bool needsNewPageDirectory = managementData->numPages > maxEntriesInPD * managementData->numPageDP;

if (needsNewPageDirectory) {
    // Increment the page and directory counts
    managementData->numPages++;
    managementData->numPageDP++;

    // Allocate and initialize memory for the new page directory
    PageDirectoryEntry *newPageDirectory = (PageDirectoryEntry *)malloc(PAGE_SIZE);
    if (newPageDirectory) {
        memset(newPageDirectory, 0, PAGE_SIZE);

        // Create a handle for the new page directory to be written to disk
        SM_PageHandle pageDirectoryHandle = (SM_PageHandle)malloc(PAGE_SIZE);
        if (pageDirectoryHandle) {
            memcpy(pageDirectoryHandle, newPageDirectory, PAGE_SIZE);
            memset(pageDirectoryHandle + sizeof(PageDirectoryEntry), 0, PAGE_SIZE - sizeof(PageDirectoryEntry));

            // Write the new page directory to the specified block
            writeBlock(managementData->numPages + 1, &managementData->fileHndl, pageDirectoryHandle);

            // Clean up allocated memory for the handle
            free(pageDirectoryHandle);
        }
        // Free the allocated memory for the new page directory
        free(newPageDirectory);
    }
}

// Initialize page number to an invalid state
int i = 0; // Initialize the index
int pageNum = -1; // Initialize pageNum to -1

// Find a page with free space using a while loop
while (i <= managementData->numPages - managementData->numPageDP) {
    if (managementData->pageDirectory[i].hasFreeSlot) {
        pageNum = i; // Found a page with free space
        break; // Exit the loop
    }
    i++; // Increment the index to check the next page
}


    // Check if a free page was found; if not, allocate a new page
if (pageNum == -1) {
    // Calculate the new page number and update management data
    pageNum = managementData->numPages + 1 - managementData->numPageDP;
    managementData->numPages++;

    // Resize the page directory to accommodate the new page
    int newPageDirSize = (managementData->numPages - managementData->numPageDP + 1) * sizeof(PageDirectoryEntry);
    managementData->pageDirectory = (PageDirectoryEntry*) realloc(managementData->pageDirectory, newPageDirSize);
    
    // Initialize the new page directory entry
    managementData->pageDirectory[pageNum] = (PageDirectoryEntry){
        .pageID = managementData->numPages - managementData->numPageDP,
        .hasFreeSlot = true,
        .freeSpace = PAGE_SIZE,
        .recordCount = 0
    };

    // Allocate and initialize a new page
    SM_PageHandle newPageHandle = (SM_PageHandle)malloc(PAGE_SIZE);
    // memset(newPageHandle, 0, PAGE_SIZE); // Uncomment if needed

    // Write the new page to disk
    writeBlock(managementData->numPages + 1, &managementData->fileHndl, newPageHandle);
    free(newPageHandle);
}

// Pin the newly allocated page to read from it
pinPage(&managementData->bm, &managementData->pageHndlBM, managementData->numPages + 1);
SM_PageHandle pageHandle = managementData->pageHndlBM.data;

// Locate a free slot within the page
int slotNum = -1;
for (int i = 0; i <= managementData->pageDirectory[pageNum].recordCount; i++) {
    SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageHandle + i * sizeof(SlotDirectoryEntry));
    if (slotEntry->isFree) {
        slotNum = i; // Found a free slot
        break;
    }
}

// If no free slot is found, append the record at the end
if (slotNum == -1) {
    slotNum = managementData->pageDirectory[pageNum].recordCount++;
}

// Calculate the offset for the new record data
int recordOffset = PAGE_SIZE - (managementData->pageDirectory[pageNum].recordCount * recordSize);

// Update the slot directory entry with the new record's details
SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageHandle + slotNum * sizeof(SlotDirectoryEntry));
slotEntry->offset = recordOffset;
slotEntry->isFree = false;

// Copy the record data into the page at the calculated offset
memcpy(pageHandle + recordOffset, record->data, recordSize);

// Update the record's RID with the current page and slot details
record->id.page = managementData->pageDirectory[pageNum].pageID;
record->id.slot = slotNum;

// Adjust free space and slot availability in the page directory entry
managementData->pageDirectory[pageNum].freeSpace -= (recordSize + sizeof(SlotDirectoryEntry));
if (managementData->pageDirectory[pageNum].freeSpace < (recordSize + sizeof(SlotDirectoryEntry))) {
    managementData->pageDirectory[pageNum].hasFreeSlot = false;
}

    // Mark the page as dirty and unpin it
    markDirty(&managementData->bm, &managementData->pageHndlBM);
    unpinPage(&managementData->bm, &managementData->pageHndlBM);

    // Write the modified page back to the file
    writeBlock(managementData->numPages + 1, &managementData->fileHndl, pageHandle);

    // Write the modified page back to the file
    SM_PageHandle pageDirectoryHandle = (SM_PageHandle) malloc(PAGE_SIZE);
    //memset(pageDirectoryHandle, 0, PAGE_SIZE);
    memcpy(pageDirectoryHandle, &managementData->numPages, sizeof(int));
    memcpy(pageDirectoryHandle + sizeof(int), &managementData->numPageDP, sizeof(int));

    // Copy the modified page directory entries
    memcpy(pageDirectoryHandle + 2 * sizeof(int), &managementData->pageDirectory[managementData->numPageDP - 1], PAGE_SIZE - 2 * sizeof(int));
    writeBlock((managementData->numPages / maxEntriesInPD) * maxEntriesInPD + managementData->numPageDP, &managementData->fileHndl, pageDirectoryHandle);
    free(pageDirectoryHandle);

    return RC_OK;
}

/*
 * Deletes a record from the table.
 * This function deletes a record specified by its RID from the table.
 *
 * Parameters:
 * - rel: Pointer to the RM_TableData structure representing the table.
 * - id: The Record ID (RID) of the record to be deleted.
 *
 * Returns:
 * - RC_OK: The record was deleted successfully.
 * - RC_RM_INVALID_RID: The provided RID is invalid.
 * - RC_RM_RECORD_NOT_FOUND: The record with the provided RID was not found.
 */
extern RC deleteRecord(RM_TableData *rel, RID id) {
    RM_managementData *managementData = (RM_managementData *) rel->managementData;

    // Check if the RID is valid
    if (id.page < 0 || id.page >= managementData->numPages || id.slot < 0) {
        return RC_RM_INVALID_RID;
    }

    // Read the page from the file
    RC rc = pinPage(&managementData->bm, &managementData->pageHndlBM, id.page + managementData->numPageDP + 1);
    if (rc != RC_OK) {
        return rc; // Error handling for pinPage failure
    }
    
    SM_PageHandle pageHandle = managementData->pageHndlBM.data;

    // Get the slot directory entry for the record
    SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageHandle + id.slot * sizeof(SlotDirectoryEntry));

    // Check if the slot is already free (record does not exist)
    if (slotEntry->isFree) {
        unpinPage(&managementData->bm, &managementData->pageHndlBM);
        return RC_RM_RECORD_NOT_FOUND;
    }

    // Mark the slot as free
    slotEntry->isFree = true;

    // Update the page directory entry
    managementData->pageDirectory[id.page].freeSpace += (slotEntry->offset - (id.slot * sizeof(SlotDirectoryEntry)));
    managementData->pageDirectory[id.page].hasFreeSlot = true;

    // Mark the page as dirty and unpin it
    markDirty(&managementData->bm, &managementData->pageHndlBM);
    
    rc = unpinPage(&managementData->bm, &managementData->pageHndlBM);
    if (rc != RC_OK) {
        return rc; // Error handling for unpinPage failure
    }

    return RC_OK;
}


/*
 * Updates a record in the table.
 * This function updates the data of a record in the table with the provided record.
 * If the updated record does not fit in the original slot, it is treated as a new record.
 *
 * Parameters:
 * - rel: Pointer to the RM_TableData structure representing the table.
 * - record: Pointer to the Record structure containing the updated data and the RID of the record to be updated.
 *
 * Returns:
 * - RC_OK: The record was updated successfully.
 * - RC_RM_INVALID_RID: The provided RID is invalid.
 * - RC_RM_RECORD_NOT_FOUND: The record with the provided RID was not found.
 */
extern RC updateRecord(RM_TableData *rel, Record *record) {
    RM_managementData *managementData = (RM_managementData *) rel->managementData;

    // Check if the RID is valid
    if (record->id.page < 0 || record->id.page >= managementData->numPages || record->id.slot < 0) {
        return RC_RM_INVALID_RID;
    }

    // Read the page from the file
    RC rc = pinPage(&managementData->bm, &managementData->pageHndlBM, record->id.page + managementData->numPageDP + 1);
    if (rc != RC_OK) {
        return rc; // Error handling for pinPage failure
    }
    
    SM_PageHandle pageData = managementData->pageHndlBM.data;

// Access the slot directory entry based on the record's slot ID
SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageData + (record->id.slot * sizeof(SlotDirectoryEntry)));

// Verify if the slot is marked as free, meaning the record is missing
if (slotEntry->isFree) {
    unpinPage(&managementData->bm, &managementData->pageHndlBM);
    return RC_RM_RECORD_NOT_FOUND;
}

// Determine the size of the record
int recSize = getRecordSize(rel->schema);

// Calculate available space, including the difference from the existing record's slot
int availableSpace = managementData->pageDirectory[record->id.page].freeSpace + 
                     (slotEntry->offset - (record->id.slot * sizeof(SlotDirectoryEntry)));

// Check if the new record fits in the existing page space
// Determine the action based on record size compared to available space
int action = (recSize > availableSpace) ? 1 : 0; // 1 for delete and insert, 0 for update

switch (action) {
    case 1: { // Delete and insert
        // Remove the current record and insert a new one if it doesn't fit
        RC resultCode = deleteRecord(rel, record->id);
        if (resultCode != RC_OK) {
            unpinPage(&managementData->bm, &managementData->pageHndlBM);
            return resultCode; // Error handling for deleteRecord failure
        }

        resultCode = insertRecord(rel, record);
        if (resultCode != RC_OK) {
            unpinPage(&managementData->bm, &managementData->pageHndlBM);
            return resultCode; // Error handling for insertRecord failure
        }
        break; // Break out of switch after handling the case
    }
    case 0: { // Update existing record
        // Update the existing record within the page
        void *destination = pageData + slotEntry->offset;  // Calculate destination pointer
        memcpy(destination, record->data, recSize);         // Copy the new record data

        // Adjust the free space in the page directory
        int pageIndex = record->id.page;                    // Get the page index
        int slotOffset = slotEntry->offset;                  // Get the slot offset
        int slotSize = record->id.slot * sizeof(SlotDirectoryEntry); // Calculate the size of the slot
        managementData->pageDirectory[pageIndex].freeSpace -= (recSize - (slotOffset - slotSize));
        break; // Break out of switch after handling the case
    }
    default:
        return RC_UNEXPECTED_ACTION; // Handle unexpected action state if necessary
}


// Mark the page as modified (dirty)
markDirty(&managementData->bm, &managementData->pageHndlBM);


    // Unpin the page
    rc = unpinPage(&managementData->bm, &managementData->pageHndlBM);
    if (rc != RC_OK) {
        return rc; // Error handling for unpinPage failure
    }

    return RC_OK;
}


extern RC getRecord(RM_TableData *table, RID recordID, Record *resultRecord) {
    RM_managementData *mgmtData = (RM_managementData *)table->managementData;

    // Validate the given Record ID (RID)
    if (!(recordID.page >= 0 && recordID.page < mgmtData->numPages && recordID.slot >= 0)) {
        return RC_RM_INVALID_RID;
    }

    // Pin the appropriate page in the buffer pool
    int targetPage = recordID.page + mgmtData->numPageDP + 1;
    RC pinStatus = pinPage(&mgmtData->bm, &mgmtData->pageHndlBM, targetPage);
    if (pinStatus != RC_OK) {
        return pinStatus; // Return immediately if page pinning fails
    }

    // Access the data in the pinned page
    SM_PageHandle pageContent = mgmtData->pageHndlBM.data;
    SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageContent + (recordID.slot * sizeof(SlotDirectoryEntry)));

    // Handle the case when the slot is not occupied
    switch (slotEntry->isFree) {
        case true:
            unpinPage(&mgmtData->bm, &mgmtData->pageHndlBM);
            return RC_RM_RECORD_NOT_FOUND;
        default:
            break;
    }

    // Fetch the size of the record based on the schema
    int recSize = getRecordSize(table->schema);

    // Copy the record data into the result record
    resultRecord->id = recordID;
    memcpy(resultRecord->data, pageContent + slotEntry->offset, recSize);

    // Safely unpin the page and check for any issues
    RC unpinStatus = unpinPage(&mgmtData->bm, &mgmtData->pageHndlBM);
    return unpinStatus == RC_OK ? RC_OK : unpinStatus;
}


/*
 * Initializes a scan operation on the table based on the given condition.
 * This function initializes the RM_ScanHandle data structure passed as an argument to startScan.
 *
 * Parameters:
 * - rel: Pointer to the RM_TableData structure representing the table.
 * - scan: Pointer to the RM_ScanHandle structure to be initialized.
 * - cond: Pointer to the Expr structure representing the scan condition.
 *
 * Returns:
 * - RC_OK: Scan initialization was successful.
 */
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    // Check for null pointers
    if (rel == NULL || scan == NULL) {
        return RC_RM_NULL_POINTER; // Error handling for null input parameters
    }

   // Allocate and initialize memory for the scan info
ScanInfo *scanInfo = (ScanInfo *)malloc(sizeof(ScanInfo));
if (!scanInfo) {
    return RC_MEM_ALLOCATION_FAIL; // Error handling for malloc failure
}

// Initialize the scan info and management data
*scanInfo = (ScanInfo){ .condition = cond, .currentPage = 0, .currentSlot = 0 }; // Use compound literal for initialization
scan->rel = rel; 
scan->mgmtData = scanInfo;


    RM_managementData *managementData = (RM_managementData *)rel->managementData;

    // Read the first page of the table (data pages start at page 2)
    RC rc = pinPage(&managementData->bm, &managementData->pageHndlBM, 2);
    if (rc != RC_OK) {
        free(scanInfo); // Free allocated memory on failure
        return rc; // Error handling for pinPage failure
    }

    return RC_OK; // Scan initialization successful
}



int ceiling(int numerator, int denominator) {
    int quotient = numerator / denominator;
    if (numerator % denominator != 0) {
        quotient += 1;
    }
    return quotient;
}


extern RC next (RM_ScanHandle *scan, Record *record) {
    // Extract relevant data from scan and rel objects
ScanInfo *scanInfo = (ScanInfo *)scan->mgmtData;
RM_TableData *rel = scan->rel;
RM_managementData *managementData = (RM_managementData *)rel->managementData;

// Calculate maximum entries and record size in a more streamlined manner
int maxEntriesInPD = (PAGE_SIZE - (2 * sizeof(int))) / sizeof(PageDirectoryEntry), recordSize = getRecordSize(rel->schema);

    // Loop to find the next available record
    for (; scanInfo->currentPage <= managementData->numPages - managementData->numPageDP; scanInfo->currentPage++) {

        int pageNumPin = ceiling(scanInfo->currentPage + 1, maxEntriesInPD) + 1 + scanInfo->currentPage;
        pinPage(&managementData->bm, &managementData->pageHndlBM, pageNumPin);
        SM_PageHandle pageHandle = managementData->pageHndlBM.data;

        // Loop through slots on the current page
        // Iterate through the slots in the current page
for (int slotIdx = scanInfo->currentSlot; 
     slotIdx < managementData->pageDirectory[scanInfo->currentPage].recordCount; 
     slotIdx++) {

    SlotDirectoryEntry *slotEntry = (SlotDirectoryEntry *)(pageHandle + slotIdx * sizeof(SlotDirectoryEntry));
    
    // Only process non-free slots
    if (slotEntry->isFree) continue;

    // Set record ID based on current page and slot
    record->id.page = scanInfo->currentPage;
    record->id.slot = slotIdx;

    // Copy record data into the record structure
    // Copy record data to record->data
    void *source = pageHandle + slotEntry->offset;
    void *destination = record->data;
    memcpy(destination, source, recordSize);

    // Perform condition evaluation
    Value *result = NULL;
    evalExpr(record, rel->schema, scanInfo->condition, &result);


    // Check if the record meets the condition or if there is no condition
    bool shouldReturn = (result->v.boolV == TRUE) || (scanInfo->condition == NULL);
    freeVal(result);  // Free evaluation result

    if (shouldReturn) {
        unpinPage(&managementData->bm, &managementData->pageHndlBM);
        scanInfo->currentSlot = slotIdx + 1;  // Increment for the next call
        return RC_OK; // Return successfully if condition is met
        }
    }

        // If no valid record is found, the loop will exit here.

        // Reset slot and move to next page
        scanInfo->currentSlot = 0;
        unpinPage(&managementData->bm, &managementData->pageHndlBM);
    }

    return RC_RM_NO_MORE_TUPLES;
}


/*
 * Closes the scan operation and releases associated resources.
 *
 * Parameters:
 * - scan: Pointer to the scan handle.
 *
 * Returns:
 * - RC_OK: If the scan is successfully closed.
 */
extern RC closeScan(RM_ScanHandle *scan) {
    // Return success if the scan handle is NULL, nothing to close
    if (!scan) return RC_OK;

    // Validate and extract scan management data
    ScanInfo *scanInfo = (ScanInfo *) scan->mgmtData;
    if (!scanInfo) return RC_RM_SCAN_INFO_NULL; // Return error if scanInfo is NULL

   RM_managementData *managementData = (RM_managementData *) scan->rel->managementData;

    // Safely unpin the current page if it was pinned
    if (scanInfo->currentPage >= 0 && scanInfo->currentPage < managementData->numPages) {
        RC unpinResult = unpinPage(&managementData->bm, &managementData->pageHndlBM);
        if (unpinResult != RC_OK) {
            // Log the error if unpinning fails (assuming a logging function exists)
            // logError("Failed to unpin page during closeScan.");
            return unpinResult; // Return the error from unpinning
        }
    }

    // Clean up by freeing the memory for ScanInfo
    free(scanInfo);
    scan->mgmtData = NULL; // Prevent dangling pointer

    // Optionally free the RM_ScanHandle if required
    // free(scan); // Uncomment if you want to release the scan handle

    return RC_OK; // Return success
}

extern int getRecordSize(Schema *schema) {
    // Return the size in bytes of records for a given schema
    int recordSize = 0, numAttr = schema->numAttr;


    for (int i = 0; i < numAttr; i++) {
        if (schema->dataTypes[i] == DT_INT) {
            recordSize += sizeof(int);
        } else if (schema->dataTypes[i] == DT_FLOAT) {
            recordSize += sizeof(float);
        } else if (schema->dataTypes[i] == DT_STRING) {
            recordSize += schema->typeLength[i];
        } else if (schema->dataTypes[i] == DT_BOOL) {
            recordSize += sizeof(bool);
        } else {
            // Handle unsupported data types if necessary
        }
    }

    return recordSize;
}


// Creates a schema with the provided attributes and keys
extern Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    // Log the start of the schema creation process
    printf("Creating schema with %d attributes and key size %d.\n", numAttr, keySize);

    // Allocate memory for the schema structure
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    if (schema == NULL) {
        printf("Error: Memory allocation for schema failed.\n");
        return NULL; // Return NULL if allocation fails
    }

    // Set number of attributes in the schema
    schema->numAttr = numAttr;

    // Allocate memory for attribute names
    schema->attrNames = (char **)malloc(numAttr * sizeof(char *));
    if (schema->attrNames == NULL) {
        printf("Error: Memory allocation for attribute names failed.\n");
        free(schema); // Free previously allocated memory
        return NULL; // Return NULL if allocation fails
    }

    // Allocate memory for data types
    schema->dataTypes = (DataType *)malloc(numAttr * sizeof(DataType));
    if (schema->dataTypes == NULL) {
        printf("Error: Memory allocation for data types failed.\n");
        free(schema->attrNames); // Free attribute names
        free(schema); // Free schema
        return NULL; // Return NULL if allocation fails
    }

    // Allocate memory for type lengths
    schema->typeLength = (int *)malloc(numAttr * sizeof(int));
    if (schema->typeLength == NULL) {
        printf("Error: Memory allocation for type lengths failed.\n");
        free(schema->dataTypes); // Free data types
        free(schema->attrNames); // Free attribute names
        free(schema); // Free schema
        return NULL; // Return NULL if allocation fails
    }

    // Set key size
    schema->keySize = keySize;

    // Allocate memory for key attributes
    schema->keyAttrs = (int *)malloc(keySize * sizeof(int));
    if (schema->keyAttrs == NULL) {
        printf("Error: Memory allocation for key attributes failed.\n");
        free(schema->typeLength); // Free type lengths
        free(schema->dataTypes); // Free data types
        free(schema->attrNames); // Free attribute names
        free(schema); // Free schema
        return NULL; // Return NULL if allocation fails
    }

    // Copy attribute names, data types, and type lengths into the schema
    for (int i = 0; i < numAttr; i++) {
        schema->attrNames[i] = strdup(attrNames[i]);
        if (schema->attrNames[i] == NULL) {
            printf("Error: Memory allocation for attribute name %s failed.\n", attrNames[i]);
            // Free all previously allocated memory
            if (schema != NULL) {
    // Deallocate attribute names if present
    if (schema->attrNames != NULL) {
        for (int p = 0; p < i; ++p) {
            free(schema->attrNames[p]);
        }
        free(schema->attrNames);
    }

    // Free remaining schema components
    free(schema->typeLength);
    free(schema->dataTypes);
    free(schema->keyAttrs);
    free(schema);
}
            return NULL; // Return NULL if allocation fails
        }
        schema->dataTypes[i] = dataTypes[i];
        schema->typeLength[i] = typeLength[i];
    }

    // Copy key attributes into the schema
    for (int i = 0; i < keySize; i++) {
        schema->keyAttrs[i] = keys[i];
    }

    // Log successful schema creation
    printf("Schema created successfully.\n");

    return schema; // Return the created schema
}


/*
 * Frees the memory allocated for a schema.
 *
 * Parameters:
 * - schema: Pointer to the schema to be freed.
 *
 * Returns:
 * - RC_OK if successful, or an error code if an error occurs.
 */
extern RC freeSchema(Schema *schema) {
    // Check if the schema pointer is NULL
    if (schema == NULL) {
        // Log a message indicating that the schema is already NULL
        printf("Schema pointer is NULL. Nothing to free.\n");
        return RC_OK; // Nothing to free, return success
    }

    // Log the number of attributes in the schema before freeing
    printf("Freeing schema with %d attributes.\n", schema->numAttr);

    // Free the memory for attribute names
    for (int i = 0; i < schema->numAttr; i++) {
        if (schema->attrNames[i] != NULL) {
            free(schema->attrNames[i]); // Free each attribute name
            schema->attrNames[i] = NULL; // Set pointer to NULL after freeing
        } else {
            printf("Warning: Attempted to free a NULL attribute name pointer at index %d.\n", i);
        }
    }
    
    // Free the memory allocated for the array of attribute names
    free(schema->attrNames);
    schema->attrNames = NULL; // Set pointer to NULL after freeing

    // Free the memory for data types
    if (schema->dataTypes != NULL) {
        free(schema->dataTypes); // Free the data types array
        schema->dataTypes = NULL; // Set pointer to NULL after freeing
    }

    // Free the memory for type lengths
    if (schema->typeLength != NULL) {
        free(schema->typeLength); // Free the type lengths array
        schema->typeLength = NULL; // Set pointer to NULL after freeing
    }

    // Free the memory for key attributes
    if (schema->keyAttrs != NULL) {
        free(schema->keyAttrs); // Free the key attributes array
        schema->keyAttrs = NULL; // Set pointer to NULL after freeing
    }

    // Free the memory for the schema structure itself
    free(schema);
    schema = NULL; // Set pointer to NULL after freeing

    // Log completion of schema freeing
    printf("Schema freed successfully.\n");

    return RC_OK; // Return success
}


/*
 * Creates a new record based on the provided schema.
 *
 * Parameters:
 * - record: Pointer to a pointer that will store the newly created record.
 * - schema: Pointer to the schema based on which the record will be created.
 *
 * Returns:
 * - RC_OK if successful, RC_RM_MALLOC_FAILED if memory allocation fails.
 */
extern RC createRecord(Record **record, Schema *schema) {
    // Validate input parameters
    if (record == NULL || schema == NULL) {
        // Log an error if the input pointers are NULL
        printf("Error: Null pointer passed for record or schema.\n");
        return RC_RM_NULL_POINTER; // Assuming this is a defined error code for NULL pointers
    }

    // Allocate memory for a new Record structure
    Record *newRecord = (Record *) malloc(sizeof(Record));
    
    // Check if memory allocation for the Record was successful
    if (newRecord == NULL) {
        // Log memory allocation failure
        printf("Error: Memory allocation for Record failed.\n");
        return RC_RM_MALLOC_FAILED;
    }

    // Initialize the Record ID to an invalid state
    newRecord->id.page = -1;  // Page ID set to -1 to indicate no page assigned
    newRecord->id.slot = -1;  // Slot ID set to -1 to indicate no slot assigned
    newRecord->data = NULL;    // Initialize data pointer to NULL

    // Get the size of the record based on the provided schema
    size_t recordSize = getRecordSize(schema);
    
    // Check if the record size is valid
    if (recordSize == 0) {
        // Log an error if the record size is zero, which is unexpected
        printf("Error: Record size calculated as zero.\n");
        free(newRecord); // Free the allocated memory for the Record structure
        return RC_RM_DATA_SIZE_ERROR; // Assuming this is a defined error code for invalid size
    }

    // Allocate memory for the record's data buffer based on the schema size
    newRecord->data = (char *) malloc(recordSize);
    
    // Check if memory allocation for the record's data was successful
    if (newRecord->data == NULL) {
        // Log memory allocation failure for data
        printf("Error: Memory allocation for Record data failed.\n");
        free(newRecord); // Free the previously allocated record structure
        return RC_RM_MALLOC_FAILED;
    }

    // Initialize the record's data buffer to zero
    memset(newRecord->data, 0, recordSize);

    // Log successful record creation
    printf("Successfully created a new record at address %p with size %zu bytes.\n", (void *)newRecord, recordSize);

    // Assign the newly created record to the output pointer
    *record = newRecord; // Set the output pointer to point to the new record

    // Return success status
    return RC_OK; // Indicate that the record was created successfully
}


/*
 * Frees the memory allocated for a record.
 *
 * Parameters:
 * - record: Pointer to the record whose memory is to be freed.
 *
 * Returns:
 * - RC_OK if successful, or if the provided record pointer is NULL.
 */
extern RC freeRecord (Record *record) {
    // Check if the record pointer is NULL
    if (record == NULL) {
        // If the record is NULL, there is nothing to free, return success
        return RC_OK;
    }

    // Log the action of freeing the record
    printf("Freeing record at address %p\n", (void *)record);

    // Check if the record's data pointer is valid before freeing
    if (record->data != NULL) {
        // Free the memory allocated for the record's data
        free(record->data);
        record->data = NULL; // Set pointer to NULL after freeing for safety
    } else {
        // Log if the record's data was already NULL
        printf("Record's data pointer is already NULL at address %p\n", (void *)record);
    }

    // Free the record structure itself
    free(record);
    record = NULL; // Set pointer to NULL after freeing for safety

    // Log that the record has been successfully freed
    printf("Record successfully freed.\n");

    // Return success status
    return RC_OK;
}


// Sets the attribute values of a record for a given schema
extern RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {

    // Validate attribute number and data type
if (attrNum < 0 || attrNum >= schema->numAttr || value->dt != schema->dataTypes[attrNum]) {
    return (attrNum < 0 || attrNum >= schema->numAttr) 
            ? RC_RM_INVALID_ATTRIBUTE // Invalid attribute number
            : RC_RM_ATTRIBUTE_TYPE_MISMATCH; // Type mismatch error
}


    // Calculate the offset of the attribute within the record's data buffer based on schema
    int attrOffset = 0;
    for (int i = 0; i < attrNum; i++) {
        if (schema->dataTypes[i] == DT_INT) {
            attrOffset += sizeof(int);
        } else if (schema->dataTypes[i] == DT_FLOAT) {
            attrOffset += sizeof(float);
        } else if (schema->dataTypes[i] == DT_BOOL) {
            attrOffset += sizeof(bool);
        } else if (schema->dataTypes[i] == DT_STRING) {
            attrOffset += schema->typeLength[i];
        } else {
            // Handle unsupported data types
            return RC_RM_DATA_TYPE_ERROR; // Unsupported data type error
        }
    }

    // Copy the attribute value from the Value structure to the record's data buffer
    if (schema->dataTypes[attrNum] == DT_INT) {
        memcpy(record->data + attrOffset, &(value->v.intV), sizeof(int));
    } else if (schema->dataTypes[attrNum] == DT_FLOAT) {
        memcpy(record->data + attrOffset, &(value->v.floatV), sizeof(float));
    } else if (schema->dataTypes[attrNum] == DT_BOOL) {
        memcpy(record->data + attrOffset, &(value->v.boolV), sizeof(bool));
    } else if (schema->dataTypes[attrNum] == DT_STRING) {
        strncpy(record->data + attrOffset, value->v.stringV, schema->typeLength[attrNum]);
    } else {
        // Handle unsupported data types
        return RC_RM_DATA_TYPE_ERROR; // Unsupported data type error
    }

    return RC_OK; // Attribute value set successfully
}


// Retrieves the attribute value of a record for a given schema.
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value) {
    // Calculate the offset of the attribute within the record's data buffer based on schema
    int attrOffset = 0;

    // Calculate the offset for the attribute specified by attrNum
    int i = 0;
while (i < attrNum) {
    if (schema->dataTypes[i] == DT_INT) {
        attrOffset += sizeof(int);
    } else if (schema->dataTypes[i] == DT_FLOAT) {
        attrOffset += sizeof(float);
    } else if (schema->dataTypes[i] == DT_STRING) {
        attrOffset += schema->typeLength[i];
    } else if (schema->dataTypes[i] == DT_BOOL) {
        attrOffset += sizeof(bool);
    } else {
        // Handle unsupported data types, if necessary
        return RC_RM_DATA_TYPE_ERROR; // or an appropriate error code
    }
    i++;
}


    // Allocate memory for the Value structure to store the attribute value
    *value = (Value *)malloc(sizeof(Value));
    if (*value == NULL) {
        return RC_MALLOC_ERROR; // or an appropriate error code
    }

    // Set the data type of the attribute value
    (*value)->dt = schema->dataTypes[attrNum];

    // Copy the attribute value from the record's data buffer to the Value structure
    if ((*value)->dt == DT_INT) {
        memcpy(&((*value)->v.intV), record->data + attrOffset, sizeof(int));
    } else if ((*value)->dt == DT_FLOAT) {
        memcpy(&((*value)->v.floatV), record->data + attrOffset, sizeof(float));
    } else if ((*value)->dt == DT_STRING) {
        char *allocatedString = (char *)malloc(schema->typeLength[attrNum] + 1);
    if (allocatedString == NULL) {
        free(*value); // Free the previously allocated memory
        return RC_MALLOC_ERROR; // Return an error if memory allocation fails
    }

        (*value)->v.stringV = allocatedString; // Assign the allocated memory to stringV
       int length = schema->typeLength[attrNum];
        memcpy((*value)->v.stringV, record->data + attrOffset, length);
        (*value)->v.stringV[length] = '\0'; // Ensure the string is null-terminated

    } else if ((*value)->dt == DT_BOOL) {
        memcpy(&((*value)->v.boolV), record->data + attrOffset, sizeof(bool));
    } else {
        // Handle unsupported data types
        free(*value);
        return RC_RM_DATA_TYPE_ERROR; // or an appropriate error code
    }

    return RC_OK; // Attribute value retrieved successfully
}


