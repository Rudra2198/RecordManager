# RECORD MANAGER

The Record Manager in C is responsible for handling the organization and management of records in a table. It offers functionality for creating, opening, and closing tables, as well as managing individual records within these tables. It also includes features for scanning records based on certain conditions, updating schemas, and managing attribute values.


## HOW TO RUN THE MAKEFILE

**Step-by-step instructions for running the Record Manager using the Makefile:**

1. Open a terminal in the project directory.
   
2. Compile the program by typing the following command in the terminal:

   make
   
   make run
   
   make clean

## CONTRIBUTION

Rudra Patel

Sabarish Raja

Enoch Ashong

Each group member has made an equal contribution to the completion of the assignment 3. Each member has worked on test cases, record manager functionality, and documentation, thus everyone has equal contribution.

## FILE STRUCTURE

* assign3

  * #### Makefile: Automates the build process.

  * #### README.txt: Documentation for the project.

  * #### buffer_mgr.c / buffer_mgr.h: Buffer Manager implementation and header files.

  * #### buffer_mgr_stat.c / buffer_mgr_stat.h: Helper functions for buffer statistics.

  * #### dberror.c / dberror.h: Error handling functions.

  * #### storage_mgr.c / storage_mgr.h: Storage Manager implementation.

  * #### record_mgr.c / record_mgr.h: Record Manager implementation and header files.

  * #### test_assign3_1.c / test_assign3_2.c: Test files for Record Manager.

  * #### expr.c / expr.h: Expression evaluation for conditions.

  * #### rm_serializer.c / rm_serializer.h: Serializer for Record Manager operations.

  * #### dt.h: Defines data types.

## RECORD MANAGER FUNCTIONS

## initRecordManager
Initializes the Record Manager.

1. Initialize the storage manager.
2. Return RC_OK to indicate successful initialization.

## shutdownRecordManager
Shuts down the Record Manager.

1. Print a shutdown message.
2. Return RC_OK to indicate successful shutdown.

## createTable
Creates a new table with the specified name and schema.

1. Validate input parameters (name and schema).
2. Create a new page file with the given name.
3. Open the newly created page file.
4. Allocate memory for the Table Information page.
5. Write schema information to the page:
   - Number of attributes
   - Attribute names
   - Data types
   - Type lengths
   - Key size and attributes
6. Write the Table Information page to the file.
7. Initialize and write the page directory:
   - Number of pages
   - Number of directory pages
   - First page entry
8. Close the file handle.
9. Return RC_OK on successful creation.

## openTable
Opens an existing table.

1. Allocate memory for RM_TableData structure.
2. Open the page file associated with the table.
3. Initialize the buffer pool with LRU replacement strategy.
4. Load the Table Information page:
   - Pin the first page
   - Copy data to in-memory page
   - Unpin the page
5. Extract schema information from the in-memory page:
   - Number of attributes
   - Attribute names
   - Data types
   - Type lengths
   - Key size and attributes
6. Load the page directory:
   - Pin the second page
   - Extract number of pages and directory pages
   - Allocate memory for page directory
   - Copy page directory data
7. Free temporary memory.
8. Return RC_OK on successful opening.

## closeTable
Closes the specified table.

1. Check if the table and its components are valid.
2. Free the schema information:
   - Attribute names
   - Data types
   - Type lengths
   - Key attributes
3. Free the page directory.
4. Shutdown the buffer pool.
5. Close the table file.
6. Free the management data.
7. Return RC_OK on successful closure.

## deleteTable
Deletes the specified table.

1. Validate the table name.
2. Destroy the page file associated with the table.
3. Return RC_OK on successful deletion.

## getNumTuples
Gets the total number of tuples stored in the table.

1. Validate the relation and management data.
2. Initialize a counter for total tuples.
3. Traverse each page in the directory:
   - Sum up the record count from each page
4. Return the total number of tuples.

## insertRecord
Inserts a new record into the table.

1. Check if a new page directory is required.
2. Find a page with free space or allocate a new page if needed.
3. Pin the target page.
4. Find a free slot within the page.
5. Write the record data to the page.
6. Update the record ID.
7. Update page directory entry (free space, slot availability).
8. Mark the page as dirty and unpin it.
9. Write the modified page back to the file.
10. Update and write the page directory.
11. Return RC_OK on successful insertion.

## deleteRecord
Deletes a record from the table.

1. Validate the provided RID.
2. Pin the page containing the record.
3. Check if the slot is already free.
4. Mark the slot as free.
5. Update the page directory entry.
6. Mark the page as dirty and unpin it.
7. Return RC_OK on successful deletion.

## updateRecord
Updates a record in the table.

1. Validate the provided RID.
2. Pin the page containing the record.
3. Check if the record exists.
4. Determine if the updated record fits in the original slot.
5. If it fits, update the record in place.
6. If it doesn't fit, delete the old record and insert the new one.
7. Update page directory entry if necessary.
8. Mark the page as dirty and unpin it.
9. Return RC_OK on successful update .

## getRecord
Gets a record from the table based on its RID.

1. Validate the provided RID.
2. Pin the page containing the record.
3. Check if the slot is occupied.
4. Copy the record data to the result record.
5. Unpin the page.
6. Return RC_OK on successful retrieval.

## startScan
Initializes a scan operation on the table.

1. Validate input parameters.
2. Allocate memory for the scan info.
3. Initialize the scan info and management data.
4. Pin the first page of the table.
5. Return RC_OK on successful initialization.

## next
Gets the next record from the scan.

1. Extract relevant data from scan and rel objects.
2. Loop to find the next available record:
   - Pin the next page.
   - Iterate through slots on the current page.
   - Check if the slot is occupied.
   - Copy the record data to the result record.
   - Evaluate the scan condition.
   - If the condition is met, return RC_OK.
3. If no valid record is found, return RC_RM_NO_MORE_TUPLES.

## closeScan
Closes the scan operation.

1. Validate the scan handle.
2. Safely unpin the current page if it was pinned.
3. Free the memory for ScanInfo.
4. Return RC_OK on successful closure.

## getRecordSize
Gets the size of a record based on the schema.

1. Initialize the record size to 0.
2. Iterate through attributes in the schema:
   - Add the size of each attribute to the record size.
3. Return the total record size.

## createSchema
Creates a new schema.

1. Allocate memory for the schema structure.
2. Set the number of attributes.
3. Allocate memory for attribute names, data types, and type lengths.
4. Copy attribute names, data types, and type lengths into the schema.
5. Allocate memory for key attributes.
6. Copy key attributes into the schema.
7. Return the created schema.

## freeSchema
Frees the memory allocated for a schema.

1. Check if the schema pointer is NULL.
2. Free the memory for attribute names.
3. Free the memory for data types.
4. Free the memory for type lengths.
5. Free the memory for key attributes.
6. Free the memory for the schema structure.
7. Return RC_OK on successful freeing.

## createRecord
Creates a new record based on the provided schema.

1. Validate input parameters.
2. Allocate memory for the Record structure.
3. Initialize the Record ID to an invalid state.
4. Allocate memory for the record's data buffer.
5. Initialize the record's data buffer to zero.
6. Return RC_OK on successful creation.

## freeRecord
Frees the memory allocated for a record.

1. Check if the record pointer is NULL.
2. Free the memory for the record's data buffer.
3. Free the memory for the Record structure.
4. Return RC_OK on successful freeing.

## setAttr
Sets the attribute value of a record.

1. Validate attribute number and data type.
2. Calculate the offset of the attribute within the record's data buffer.
3. Copy the attribute value from the Value structure to the record's data buffer.
4. Return RC_OK on successful setting.

## getAttr
Gets the attribute value of a record.

1. Calculate the offset of the attribute within the record's data buffer.
2. Allocate memory for the Value structure.
3. Set the data type of the attribute value.
4. Copy the attribute value from the record's data buffer to the Value structure.
5. Return RC_OK on successful retrieval.
