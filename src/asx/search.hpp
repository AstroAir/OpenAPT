/**
 * Class representing a database for storing astronomical data.
 */
class Database {
public:
    /**
     * Constructor for Database class.
     * 
     * @param db_name The name of the SQLite database file.
     * @throw std::runtime_error if the database fails to open.
     */
    Database(std::string db_name);

    /**
     * Destructor for Database class.
     */
    ~Database();

    /**
     * Reads data from the database and stores it in the internal vector.
     * 
     * @throw std::runtime_error if there is an error reading from the database.
     */
    void read_from_database();

    /**
     * Inserts a new data entry into the database and adds it to the internal vector.
     * 
     * @param d The new data entry to insert.
     * @throw std::runtime_error if there is an error inserting data into the database.
     */
    void insert_data(Data d);

    /**
     * Deletes a data entry from the database and removes it from the internal vector.
     * 
     * @param name The name of the data entry to delete.
     * @throw std::runtime_error if there is an error deleting data from the database.
     */
    void delete_data(std::string name);

    /**
     * Sorts the internal vector of data entries by name.
     */
    void sort_by_name();

    /**
     * Filters the internal vector of data entries by a user-provided predicate function.
     * 
     * @param filter A lambda or function pointer that takes a Data object as its parameter and returns a bool indicating whether the object should be kept in the filtered result.
     * @return A vector containing only the data entries that satisfy the provided filter.
     */
    std::vector<Data> filter_by(std::function<bool(const Data&)> filter) const;

    /**
     * Optimizes the database by performing various optimizations.
     */
    void optimize_database();

    /**
     * Saves any modifications to the internal data vector back to the database.
     * 
     * @throw std::runtime_error if there is an error updating the database.
     */
    void save_to_database();

    /**
     * Searches for data entries in the internal vector by name.
     * 
     * @param name The name to search for.
     * @return A vector containing all data entries whose names contain the search query.
     */
    std::vector<Data> search_by_name(std::string name);

    /**
     * Searches for data entries in the internal vector by right ascension and declination.
     * 
     * @param ra The right ascension of the target object.
     * @param dec The declination of the target object.
     * @param ra_range The maximum allowed deviation in right ascension.
     * @param dec_range The maximum allowed deviation in declination.
     * @return A vector containing all data entries whose coordinates fall within the specified ranges.
     */
    std::vector<Data> search_by_ra_dec(std::string ra, std::string dec, double ra_range, double dec_range);

    /**
     * Saves the current list of data entries to a JSON file.
     * 
     * @param filename The name of the file to save the JSON data to.
     */
    void save_to_json(std::string filename);

private:
    /**
     * Callback function used for reading data from the SQLite database.
     */
    static int callback(void *data, int argc, char **argv, char **col_name);

    /**
     * Converts a string representation of a right ascension or declination coordinate to decimal degrees.
     * 
     * @param str The string representation of the coordinate to convert.
     * @return The decimal degree equivalent of the input coordinate.
     */
    double to_decimal(std::string str);

private:
    sqlite3* db; // The SQLite database handle.
    std::vector<Data> data; // The vector of data entries.

private:
    /**
     * Struct representing a data entry.
     */
    struct Data {
        std::string Name; // The name of the object.
        std::string Type; // The type of the object.
        std::string RA; // The right ascension coordinate of the object.
        std::string Dec; // The declination coordinate of the object.
        std::string Const; // The constellation that the object belongs to.
    };
};