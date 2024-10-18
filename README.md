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
## Core Table Management Functions

#### initRecordManager(void *mgmtData)
Initializes the Record Manager by setting up any necessary data structures, such as internal buffers and metadata required for managing records and tables. It prepares the system to handle subsequent record operations, ensuring that all components are ready for use. If specific management data is provided, this function will also configure the Record Manager accordingly. Proper initialization is crucial to prevent runtime errors during table operations.

#### shutdownRecordManager()
Cleans up and frees any resources used by the Record Manager, ensuring that all allocated memory is properly released and that no memory leaks occur. This function is typically called when the application is closing or when the Record Manager is no longer needed. It also closes any open tables and finalizes any pending operations, ensuring a clean exit from the record management system. Proper shutdown helps maintain system stability and reliability.

#### createTable(char *name, Schema *schema)
Creates a new table in the system with the specified name and schema. This function allocates necessary resources and structures to hold the table's data, including its records and attributes as defined in the schema. It checks for the existence of a table with the same name and returns an error if it already exists. Once created, the table can be opened for operations, allowing users to insert, retrieve, or manipulate records.

#### openTable(RM_TableData *rel, char *name)
Opens an existing table for operations such as inserting or retrieving records. This function locates the specified table by its name, loading its data into memory and preparing it for manipulation. If the table does not exist or is already open, appropriate error handling will occur. Upon successful opening, the function returns a handle to the table, allowing further operations to be performed on its records.

#### closeTable(RM_TableData *rel)
Closes the table after all operations have been completed, ensuring that any changes made to the records are saved and that the table's resources are released. This function also flushes any buffered changes to disk, preserving data integrity. It prevents further operations on the table until it is reopened. Proper closing of tables is essential to avoid data loss and ensure that resources are efficiently managed.

#### deleteTable(char *name)
Deletes a table and its associated data from the system, including all records stored within it. This function checks for the existence of the specified table, releasing all resources and memory allocated for it. If the table is currently open, it will first close it to ensure data integrity. After deletion, the table can no longer be accessed, and any operations performed on it will result in an error.

#### getNumTuples(RM_TableData *rel)
Returns the number of tuples (records) in the specified table, providing essential information for users to understand the current state of the table. This function counts the total records stored and returns this value, which can be useful for reporting and analysis purposes. Accurate record counting helps in managing the database effectively and planning for operations like insertion or deletion.

#### Record Handling Functions
insertRecord(RM_TableData *rel, Record *record)
Inserts a new record into the specified table, adding it to the end of the existing records. This function first checks if the record conforms to the table's schema, ensuring that all required attributes are present and valid. If the insertion is successful, it updates the count of tuples in the table. Handling errors gracefully is crucial, as invalid records or issues with storage could lead to data corruption.

#### deleteRecord(RM_TableData *rel, RID id)
Deletes a record identified by its Record ID (RID) from the specified table. This function locates the record within the table's storage and removes it, freeing any associated memory and updating the tuple count. If the RID does not correspond to an existing record, an error is returned. Ensuring that deletions are handled correctly is essential for maintaining data integrity within the system.

#### updateRecord(RM_TableData *rel, Record *record)
Updates an existing record in the table with new data provided in the input record. This function first locates the record using its ID, then verifies that the new data conforms to the table's schema. If successful, the existing record is replaced with the new data, and any necessary index updates are performed. Proper handling of update operations ensures that the data remains accurate and consistent throughout the table.

#### getRecord(RM_TableData *rel, RID id, Record *record)
Retrieves a record with a specific ID from the specified table and populates the provided record structure with its data. This function verifies the existence of the record before attempting to fetch it, ensuring that no invalid accesses occur. If the retrieval is successful, the data from the storage is copied into the record structure, allowing users to manipulate or display it as needed. Error handling is included to manage cases where the record does not exist.

## Scan Functions
#### startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
Starts a new scan on the specified table based on the provided condition expression. This function initializes the scan handle and prepares the internal state for scanning through records that match the specified condition. It effectively sets the starting point for fetching records and optimizes the retrieval process based on the condition given. Proper initialization of the scan is essential for ensuring accurate results during the scan operation.

#### next(RM_ScanHandle *scan, Record *record)
Fetches the next record in the scan that satisfies the specified condition. This function checks the current state of the scan handle and retrieves the next matching record, populating the provided record structure with its data. If no more records match the condition, it returns an indication that the scan has completed. Efficient retrieval and error handling are crucial for providing accurate results and maintaining the integrity of the scanning process.

#### closeScan(RM_ScanHandle *scan)
Closes the current scan, releasing any resources associated with it and preventing further record fetching. This function also ensures that any temporary data created during the scan is properly cleared. Closing scans when they are no longer needed is important for resource management and preventing memory leaks within the Record Manager. Proper handling of scan closures ensures system stability.

## Schema Functions
#### getRecordSize(Schema *schema)
Returns the size of a record based on the given schema, providing vital information for memory allocation and record management. This function calculates the total byte size required to store a record defined by the schema, considering all attributes and their data types. Accurate size calculations are essential for efficient storage management and for ensuring that records can be properly allocated in memory.

#### createSchema(int numAttr, char **attrNames, DataType *dataTypes, int typeLength, int keySize, int keys)
Creates a new schema that defines the structure of records in a table, specifying the number of attributes and their respective names and data types. This function allocates memory for the schema and initializes it with the provided parameters, allowing for customizable record structures. A well-defined schema is crucial for maintaining data integrity and consistency throughout the table's records.

#### freeSchema(Schema *schema)
Frees the memory allocated for a schema, ensuring that no memory leaks occur when the schema is no longer needed. This function deallocates all resources associated with the schema, including attribute names and data type information. Proper management of memory related to schemas is essential for maintaining overall system performance and reliability.

## Record and Attribute Management Functions
#### createRecord(Record **record, Schema *schema)
Creates a new record according to the provided schema, allocating necessary memory for its attributes and initializing them to default values. This function ensures that the record structure aligns with the schema definition, allowing for consistent data storage. Proper creation of records is essential for maintaining data integrity.
