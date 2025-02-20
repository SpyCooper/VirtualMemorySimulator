// include the necessary libraries
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <unordered_map>

// use the standard namespace
using namespace std;

// enum for each of the algorithm types the virtual memory can use
enum Algorithm
{
    FIFO,
    LRU,
    OPT,
};

// enum for the state of each of the pages
enum PageType
{
    UNUSED,
    STOLEN,
    MAPPED,
};

// struct for the values of each of the pages
struct Page
{
    int page_number;
    PageType type;
    int frame_number;
    int on_disk;
};

// struct for the values of each of the frames
struct Frame
{
    int frame_number;
    int page_number;
    int in_use;
    int dirty;
    int first_use;
    int last_use;
};

// class for the virtual memory
class VirtualMemory
{
    public:
        // variables
        int page_size;
        int num_frames;
        int num_pages;
        int num_bs_blocks;
        Algorithm algorithm;
        vector<Frame> frames;
        vector<Page> pages;
        vector<int> backing_store;
        int pages_referenced;
        int pages_mapped;
        int page_miss_instances;
        int frame_stolen_instances;
        int stolen_frames_written_to_swapspace;
        int stolen_frames_recovered_from_swapspace;

    // constructor
    VirtualMemory(int ps = 0, int nf = 0, int np = 0, int nbb = 0, Algorithm algo = Algorithm::FIFO)
    {
        page_size = ps;
        num_frames = nf;
        num_pages = np;
        num_bs_blocks = nbb;
        algorithm = algo;
        pages_referenced = 0;
        pages_mapped = 0;
        page_miss_instances = 0;
        frame_stolen_instances = 0;
        stolen_frames_written_to_swapspace = 0;
        stolen_frames_recovered_from_swapspace = 0;

        // initialize the pages and frames
        pages.resize(num_pages);
        frames.resize(num_frames);
        backing_store.resize(num_bs_blocks, -1); // initialize backing store with -1 indicating empty

        // initialize the pages
        for (int i = 0; i < num_pages; ++i)
        {
            pages[i].page_number = i;
            pages[i].type = UNUSED;
            pages[i].frame_number = -1;
            pages[i].on_disk = 0; // assuming all pages are on disk for simplicity
        }

        // initialize the frames
        for (int i = 0; i < num_frames; ++i)
        {
            frames[i].frame_number = i;
            frames[i].page_number = -1;
            frames[i].first_use = -1;
            frames[i].last_use = -1;
            frames[i].dirty = 0;
            frames[i].in_use = 0;
        }
    }

    // destructor
    ~VirtualMemory()
    {
        frames.clear();
        pages.clear();
        backing_store.clear();
    }

    // function to print the memory state
    void print_memory_state()
    {
        // print the page table in the following format
        /*
        Page Table
            0 type:STOLEN framenum:-1 ondisk:0
            1 type:UNUSED
            2 type:MAPPED framenum:3 ondisk:0
        */
        cout << "Page Table" << endl;
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (pages[i].type == PageType::UNUSED)
            {
                cout << setw(5) << i << " type:UNUSED" << endl;
            }
            else
            {
                cout << setw(5) << i << " ";
                if (pages[i].type == PageType::STOLEN)
                {
                    cout << "type:STOLEN ";
                }
                else
                {
                    cout << "type:MAPPED ";
                }
                cout << "framenum:" << pages[i].frame_number << " ondisk:" << pages[i].on_disk << endl;
            }
        }

        // print the frame table in the following format
        /*
        Frame Table
            0 inuse:0
            1 inuse:1 dirty:0 firstuse:1 lastuse:1
            2 inuse:1 dirty:1 firstuse:2 lastuse:2
        */
        cout << "Frame Table" << endl;
        for (size_t i = 0; i < frames.size(); ++i)
        {
            cout << setw(5) << i <<  " ";
            if (frames[i].in_use == 0)
            {
                cout << "inuse:0" << endl;
            }
            else
            {
                cout << "inuse:" << frames[i].in_use << " dirty:" << frames[i].dirty << " first_use:" << frames[i].first_use << " last_use:" << frames[i].last_use << endl;
            }
        }

        // print the statistics
        cout << "Pages referenced: " << pages_referenced << endl;
        cout << "Pages mapped: " << pages_mapped << endl;
        cout << "Page miss instances: " << page_miss_instances << endl;
        cout << "Frame stolen instances: " << frame_stolen_instances << endl;
        cout << "Stolen frames written to swapspace: " << stolen_frames_written_to_swapspace << endl;
        cout << "Stolen frames recovered from swapspace: " << stolen_frames_recovered_from_swapspace << endl;
    }

    // checks the number of pages mapped (needed to update the page table correctly)
    void check_pages_mapped()
    {
        // reset the pages mapped
        pages_mapped = 0;

        // check if all pages are mapped
        for (auto &page : pages)
        {
            if (page.type != UNUSED)
            {
                pages_mapped++;
            }
        }
    }

    // run the FIFO algorithm for one instruction
    void run_fifo_algorithm(const string &instruction, bool debug)
    {
        // get the operation and address
        char operation = instruction[0];
        string address_hex = instruction.substr(1);
        int address = stoi(address_hex, nullptr, 16);
        int page_number = address / page_size;

        // if debug is enabled, print the operation, address, and page number
        if (debug)
        {
            cout << "Operation: " << operation << " Address: " << address_hex << " Page number: " << page_number << endl;
        }

        // increment the pages referenced
        pages_referenced++;

        // check if the page is already in memory
        for (auto &frame : frames)
        {
            // page is in memory
            if (frame.page_number == page_number)
            {
                frame.last_use = pages_referenced;
                // set the dirty bit if write operation
                if (operation == 'w')
                {
                    frame.dirty = 1;
                }

                // if debug is enabled, print page hit
                if (debug)
                {
                    cout << "Page hit" << endl;
                }

                // return since the page is already in memory
                return;
            }
        }

        // if debug is enabled, that the page was missed (not in memory)
        if (debug)
        {
            cout << "Page miss" << endl;
        }

        // page miss
        page_miss_instances++;

        // find an empty frame
        for (auto &frame : frames)
        {
            // empty frame found
            if (frame.in_use == 0)
            {
                // if debug is enabled, print that an empty frame found
                if (debug)
                {
                    cout << "Empty frame found at frame " << frame.frame_number << endl;
                }

                // update the frame table
                frame.page_number = page_number;
                frame.first_use = pages_referenced;
                frame.last_use = pages_referenced;
                frame.in_use = 1;

                // set the dirty bit if write operation
                if (operation == 'w')
                {
                    frame.dirty = 1;

                    if (debug)
                    {
                        cout << "Dirty bit set" << endl;
                    }
                }

                // update the page table
                pages[page_number].frame_number = frame.frame_number;
                pages[page_number].type = MAPPED;
                pages[page_number].on_disk = 0;
                return;
            }
        }

        // no empty frame found, apply fifo replacement
        int oldest_frame_index = 0;
        for (size_t i = 1; i < frames.size(); ++i)
        {
            // find the oldest frame
            if (frames[i].first_use < frames[oldest_frame_index].first_use)
            {
                oldest_frame_index = i;
            }
        }

        // if debug is enabled, print that the oldest frame was found
        if (debug)
        {
            cout << "Oldest frame found at frame " << frames[oldest_frame_index].frame_number << endl;
        }

        // get the oldest frame
        Frame &oldest_frame = frames[oldest_frame_index];
        int oldest_page_number = oldest_frame.page_number;

        // write the stolen frame to swapspace if dirty
        if (oldest_frame.dirty)
        {
            backing_store[oldest_page_number] = oldest_frame.page_number;
            stolen_frames_written_to_swapspace++;

            // if debug is enabled, print that the oldest frame was stolen and written to swapspace
            if (debug)
            {
                cout << "Frame " << oldest_frame.frame_number << " stolen and written to swapspace" << endl;
            }
        }

        // update the page table for the page being replaced
        pages[oldest_page_number].type = STOLEN;
        pages[oldest_page_number].frame_number = -1;

        // if the value is dirty, write it to the backing store
        if (oldest_frame.dirty)
        {
            pages[oldest_page_number].on_disk = 1;
        }
        frame_stolen_instances++;

        // if debug is enabled, print that the stolen frame was updated in the page table
        if (debug)
        {
            cout << "Stolen frame updated in page table" << endl;
        }

        // if the page was previously written to the backing store
        if (pages[page_number].on_disk == 1 && pages[page_number].frame_number == -1)
        {
            // recover the stolen frame from swapspace
            stolen_frames_recovered_from_swapspace++;
            backing_store[page_number] = -1; // clear the backing store entry

            // if debug is enabled, print that the page was recovered from swapspace  
            if (debug)
            {
                cout << "Page " << page_number << " recovered from swapspace" << endl;
            }
        }

        // update the frame table
        oldest_frame.page_number = page_number;
        oldest_frame.first_use = pages_referenced;
        oldest_frame.last_use = pages_referenced;

        // set dirty bit if the operation is write
        if (operation == 'w')
        {
            oldest_frame.dirty = 1;
        }
        else
        {
            oldest_frame.dirty = 0;
        }
        pages[page_number].frame_number = oldest_frame.frame_number;
        pages[page_number].type = MAPPED;

        // if debug is enabled, print that the oldest frame was updated in the frame table
        if (debug)
        {
            cout << "Oldest frame updated in frame table" << endl;
        }
    }

    // run the LRU algorithm for one instruction
    void run_lru_algorithm(const string &instruction, bool debug)
    {
        // get the operation and address
        char operation = instruction[0];
        string address_hex = instruction.substr(1);
        int address = stoi(address_hex, nullptr, 16);
        int page_number = address / page_size;

        // if debug is enabled, print the operation, address, and page number
        if (debug)
        {
            cout << "Operation: " << operation << " Address: " << address_hex << " Page number: " << page_number << endl;
        }

        // increment the pages referenced
        pages_referenced++;

        // check if the page is already in memory
        for (auto &frame : frames)
        {
            // page is in memory
            if (frame.page_number == page_number)
            {
                // update the last use
                frame.last_use = pages_referenced;

                // set the dirty bit if the operation is write
                if (operation == 'w')
                {
                    frame.dirty = 1;
                }

                // if debug is enabled, print that the page was hit
                if (debug)
                {
                    cout << "Page hit" << endl;
                }

                // return since the page is already in memory
                return;
            }
        }

        // if debug is enabled, print that the page was missed (not in memory)
        if (debug)
        {
            cout << "Page miss" << endl;
        }

        // page miss
        page_miss_instances++;

        // find an empty frame
        for (auto &frame : frames)
        {
            // empty frame found
            if (frame.in_use == 0)
            {
                // if debug is enabled, print that an empty frame was found
                if (debug)
                {
                    cout << "Empty frame found at frame " << frame.frame_number << endl;
                }

                // update the frame table
                frame.page_number = page_number;
                frame.first_use = pages_referenced;
                frame.last_use = pages_referenced;
                frame.in_use = 1;

                // set the dirty bit if write operation
                if (operation == 'w')
                {
                    frame.dirty = 1;

                    // if debug is enabled, print that the dirty bit was set
                    if (debug)
                    {
                        cout << "Dirty bit set" << endl;
                    }
                }

                // update the page table
                pages[page_number].frame_number = frame.frame_number;
                pages[page_number].type = MAPPED;
                pages[page_number].on_disk = 0;
                return;
            }
        }

        // no empty frame found, apply lru replacement
        int lru_frame_index = 0;
        for (size_t i = 1; i < frames.size(); ++i)
        {
            // find the least recently used frame
            if (frames[i].last_use < frames[lru_frame_index].last_use)
            {
                lru_frame_index = i;

                // if debug is enabled, print that the least recently used frame was found
                if (debug)
                {
                    cout << "Least recently used frame found at frame " << frames[lru_frame_index].frame_number << endl;
                }
            }
        }

        // get the least recently used frame
        Frame &lru_frame = frames[lru_frame_index];
        int lru_page_number = lru_frame.page_number;

        // if the frame is dirty, write it to swapspace
        if (lru_frame.dirty)
        {
            backing_store[lru_page_number] = lru_frame.page_number;
            stolen_frames_written_to_swapspace++;

            // if debug is enabled, print that the frame was stolen and written to swapspace 
            if (debug)
            {
                cout << "Frame " << lru_frame.frame_number << " stolen and written to swapspace" << endl;
            }
        }

        // update the page table for the page being replaced
        pages[lru_page_number].type = STOLEN;
        pages[lru_page_number].frame_number = -1;
        
        // if the value is dirty, write it to the backing store
        if (lru_frame.dirty)
        {
            pages[lru_page_number].on_disk = 1;
        }
        frame_stolen_instances++;

        // if debug is enabled, print that the stolen frame was updated in the page table
        if (debug)
        {
            cout << "Stolen frame updated in page table" << endl;
        }

        // if the page was previously written to the backing store
        if (pages[page_number].on_disk == 1 && pages[page_number].frame_number == -1)
        {
            // recover the stolen frame from swapspace
            stolen_frames_recovered_from_swapspace++;
            backing_store[page_number] = -1; // clear the backing store entry

            // if debug is enabled, print that the page was recovered from swapspace
            if (debug)
            {
                cout << "Page " << page_number << " recovered from swapspace" << endl;
            }
        }

        // update the frame table
        lru_frame.page_number = page_number;
        lru_frame.first_use = pages_referenced;
        lru_frame.last_use = pages_referenced;

        // if the operation is write, set the dirty bit
        if (operation == 'w')
        {
            lru_frame.dirty = 1;
        }
        else
        {
            lru_frame.dirty = 0;
        }

        // update the page table
        pages[page_number].frame_number = lru_frame.frame_number;
        pages[page_number].type = MAPPED;

        // if debug is enabled, print that the least recently used frame was updated in the frame table
        if (debug)
        {
            cout << "Least recently used frame updated in frame table" << endl;
        }
    }

    // run the OPT algorithm for all future instructions
    void run_opt_algorithm(const vector<string> &instructions, bool debug)
    {
        // loop through all instructions
        for (size_t i = 0; i < instructions.size(); ++i)
        {
            // get the operation and address
            string instruction = instructions[i];
            char operation = instruction[0];
            string address_hex = instruction.substr(1);
            int address = stoi(address_hex, nullptr, 16);
            int page_number = address / page_size;

            // if debug is enabled, print the operation, address, and page number
            if (debug)
            {
                cout << "Operation: " << operation << " Address: " << address_hex << " Page number: " << page_number << endl;
            }

            // increment the pages referenced
            pages_referenced++;

            // check if the page is already in memory
            bool page_hit = false;
            for (auto &frame : frames)
            {
                // page is in memory
                if (frame.page_number == page_number)
                {
                    frame.last_use = pages_referenced;

                    // if the operation is write, set the dirty bit
                    if (operation == 'w')
                    {
                        frame.dirty = 1;
                    }

                    // if debug is enabled, print that the page was hit
                    if (debug)
                    {
                        cout << "Page hit" << endl;
                    }

                    page_hit = true;
                    break;
                }
            }

            // if the page is in memory, continue to the next instruction
            if (page_hit)
            {
                continue;
            }

            // if debug is enabled, print that the page was missed (not in memory)
            if (debug)
            {
                cout << "Page miss" << endl;
            }

            // page miss
            page_miss_instances++;

            // find an empty frame
            bool empty_frame_found = false;
            for (auto &frame : frames)
            {
                // empty frame found
                if (frame.in_use == 0)
                {
                    // if debug is enabled, print that an empty frame was found
                    if (debug)
                    {
                        cout << "Empty frame found at frame " << frame.frame_number << endl;
                    }

                    // update the frame table
                    frame.page_number = page_number;
                    frame.first_use = pages_referenced;
                    frame.last_use = pages_referenced;
                    frame.in_use = 1;

                    // set the dirty bit if write operation
                    if (operation == 'w')
                    {
                        frame.dirty = 1;

                        if (debug)
                        {
                            cout << "Dirty bit set" << endl;
                        }
                    }

                    // update the page table
                    pages[page_number].frame_number = frame.frame_number;
                    pages[page_number].type = MAPPED;
                    pages[page_number].on_disk = 0;
                    empty_frame_found = true;
                    break;
                }
            }

            // if an empty frame was found, continue to the next instruction
            if (empty_frame_found)
            {
                continue;
            }

            // no empty frame found, apply OPT replacement
            int opt_frame_index = -1;
            int farthest_use = -1;
            unordered_map<int, int> future_use_map;

            // find the future use of each page
            for (size_t j = i + 1; j < instructions.size(); ++j)
            {
                string future_instruction = instructions[j];
                string future_address_hex = future_instruction.substr(1);
                int future_address = stoi(future_address_hex, nullptr, 16);
                int future_page_number = future_address / page_size;
                // if the page is not in the future use map, add it
                if (future_use_map.find(future_page_number) == future_use_map.end())
                {
                    future_use_map[future_page_number] = j;
                }
            }

            // find the optimal frame
            for (size_t k = 0; k < frames.size(); ++k)
            {
                int next_use = future_use_map.count(frames[k].page_number) ? future_use_map[frames[k].page_number] : -1;

                // if the page is not used in the future, use it
                if (next_use == -1)
                {
                    opt_frame_index = k;
                    break;
                }
                else if (next_use > farthest_use)
                {
                    farthest_use = next_use;
                    opt_frame_index = k;
                }
            }

            // if debug is enabled, print that the optimal frame was found
            if (debug)
            {
                cout << "Optimal frame found at frame " << frames[opt_frame_index].frame_number << endl;
            }

            // get the optimal frame
            Frame &opt_frame = frames[opt_frame_index];
            int opt_page_number = opt_frame.page_number;

            // write the stolen frame to swapspace if dirty
            if (opt_frame.dirty)
            {
                backing_store[opt_page_number] = opt_frame.page_number;
                stolen_frames_written_to_swapspace++;

                // if debug is enabled, print that the frame was stolen and written to swapspace
                if (debug)
                {
                    cout << "Frame " << opt_frame.frame_number << " stolen and written to swapspace" << endl;
                }
            }

            // update the page table for the page being replaced
            pages[opt_page_number].type = STOLEN;
            pages[opt_page_number].frame_number = -1;
            
            // if the value is dirty, write it to the backing store
            if (opt_frame.dirty)
            {
                pages[opt_page_number].on_disk = 1;
            }
            frame_stolen_instances++;

            // if debug is enabled, print that the stolen frame was updated in the page table
            if (debug)
            {
                cout << "Stolen frame updated in page table" << endl;
            }

            // recover the stolen frame from swapspace if it was previously written
            if (pages[page_number].on_disk == 1 && pages[page_number].frame_number == -1)
            {
                stolen_frames_recovered_from_swapspace++;
                backing_store[page_number] = -1; // clear the backing store entry

                // if debug is enabled, print that the page was recovered from swapspace
                if (debug)
                {
                    cout << "Page " << page_number << " recovered from swapspace" << endl;
                }
            }

            // update the frame table
            opt_frame.page_number = page_number;
            opt_frame.first_use = pages_referenced;
            opt_frame.last_use = pages_referenced;
            
            // set the dirty bit if the operation is write
            if (operation == 'w')
            {
                opt_frame.dirty = 1;
            }
            else
            {
                opt_frame.dirty = 0;
            }
            pages[page_number].frame_number = opt_frame.frame_number;
            pages[page_number].type = MAPPED;

            // if debug is enabled, print that the optimal frame was updated in the frame table
            if (debug)
            {
                cout << "Optimal frame updated in frame table" << endl;
            }
        }
    }
};

// global variables
bool debug = false;
VirtualMemory vm = VirtualMemory(0, 0, 0, 0, FIFO);

// function prototypes
void run_opt_algorithm(const string &instruction, const vector<string> &future_instructions);
void check_pages_mapped();

// main function
int main(int argc, char *argv[])
{
    // create variables
    string filename = "";
    string algorithm_string = "";

    // check the number of arguments
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    else if (argc > 4)
    {
        cout << "Too many arguments" << endl;
        return 1;
    }
    else if (argc == 3)
    {
        algorithm_string = argv[1];
        filename = argv[2];
    }
    else if (argc == 4)
    {
        if (string(argv[1]) == "-w")
        {
            // w_flag = true;
            algorithm_string = argv[2];
            filename = argv[3];
        }
        else
        {
            cout << "Invalid argument" << endl;
            return 1;
        }
    }

    // check algorithm and set it
    Algorithm algorithm;
    if (algorithm_string == "FIFO")
    {
        algorithm = FIFO;
    }
    else if (algorithm_string == "LRU")
    {
        algorithm = LRU;
    }
    else if (algorithm_string == "OPTIMAL")
    {
        algorithm = OPT;
    }
    else
    {
        cout << "Invalid algorithm" << endl;
        return 1;
    }

    // open file
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "File not found" << endl;
        return 1;
    }

    // variables
    bool first_non_comment = false;

    // if debug is enabled, print the algorithm
    if (debug)
    {
        cout << "Algorithm: " << algorithm_string << endl;
    }

    // check if the algorithm is OPT
    if (algorithm == Algorithm::OPT)
    {

        // get the first line
        string line;
        while (getline(file, line))
        {
            if (!line.empty() && line[0] != '#')
            {
                break;
            }
        }

        // split the line to get the values
        stringstream ss(line);
        string token;
        int page_size, num_frames, num_pages, num_bs_blocks;
        // get the value of each of the variables
        if (!(ss >> token) || (page_size = stoi(token)) <= 0)
        {
            cout << "Invalid page size" << endl;
            return 1;
        }
        if (!(ss >> token) || (num_frames = stoi(token)) <= 0)
        {
            cout << "Invalid number of frames" << endl;
            return 1;
        }
        if (!(ss >> token) || (num_pages = stoi(token)) <= 0)
        {
            cout << "Invalid number of pages" << endl;
            return 1;
        }
        if (!(ss >> token) || (num_bs_blocks = stoi(token)) <= 0)
        {
            cout << "Invalid number of backing store blocks" << endl;
            return 1;
        }

        // create the virtual memory object
        vm = VirtualMemory(page_size, num_frames, num_pages, num_bs_blocks, algorithm);

        // print the values
        cout << "Page size: " << vm.page_size << endl;
        cout << "Num frames: " << vm.num_frames << endl;
        cout << "Num pages: " << vm.num_pages << endl;
        cout << "Num backing blocks: " << vm.num_bs_blocks << endl;

        // print the algorithm type
        cout << "Reclaim algorithm: " << algorithm_string << endl;

        // read all instructions into a vector
        vector<string> instructions;
        while (getline(file, line))
        {
            if (!line.empty() && line[0] != '#')
            {
                instructions.push_back(line);
            }
        }
        
        // run the OPT algorithm
        vm.run_opt_algorithm(instructions, debug);

        // check if all pages are mapped
        vm.check_pages_mapped();
    }
    // if the algorithm is FIFO or LRU
    else
    {
        // read file
        string line;
        while (getline(file, line))
        {
            if (debug)
            {
                cout << "Line: " << line << endl;
            }

            if (line.empty())
            {
                // the line is empty, so ignore it
            }
            else if (line == "debug")
            {
                // enable debugging
                debug = true;
            }
            else if (line == "nodebug")
            {
                // disable debugging
                debug = false;
            }
            else if (line == "print")
            {
                // print the output in the correct format
                vm.print_memory_state();
            }
            else
            {
                // check if the line does not start with a #
                if (line[0] != '#')
                {
                    // check if the first non-comment line has been read
                    if (first_non_comment == false)
                    {
                        // split the line to get the values
                        stringstream ss(line);
                        string token;
                        int page_size, num_frames, num_pages, num_bs_blocks;
                        if (!(ss >> token) || (page_size = stoi(token)) <= 0)
                        {
                            cout << "Invalid page size" << endl;
                            return 1;
                        }
                        if (!(ss >> token) || (num_frames = stoi(token)) <= 0)
                        {
                            cout << "Invalid number of frames" << endl;
                            return 1;
                        }
                        if (!(ss >> token) || (num_pages = stoi(token)) <= 0)
                        {
                            cout << "Invalid number of pages" << endl;
                            return 1;
                        }
                        if (!(ss >> token) || (num_bs_blocks = stoi(token)) <= 0)
                        {
                            cout << "Invalid number of backing store blocks" << endl;
                            return 1;
                        }

                        // create the virtual memory object
                        vm = VirtualMemory(page_size, num_frames, num_pages, num_bs_blocks, algorithm);

                        // print the values
                        cout << "Page size: " << vm.page_size << endl;
                        cout << "Num frames: " << vm.num_frames << endl;
                        cout << "Num pages: " << vm.num_pages << endl;
                        cout << "Num backing blocks: " << vm.num_bs_blocks << endl;

                        // print the algorithm type
                        cout << "Reclaim algorithm: " << algorithm_string << endl;

                        // set the first_non_comment to true
                        first_non_comment = true;
                    }
                    // if the first non-comment line has been read
                    else
                    {
                        // run the algorithm on that instruction
                        if (vm.algorithm == Algorithm::FIFO)
                        {
                            // FIFO algorithm
                            vm.run_fifo_algorithm(line, debug);

                            // check if all pages are mapped
                            vm.check_pages_mapped();
                        }
                        else if (vm.algorithm == Algorithm::LRU)
                        {
                            // LRU algorithm
                            vm.run_lru_algorithm(line, debug);

                            // check if all pages are mapped
                            vm.check_pages_mapped();
                        }
                    }
                }
                // if the line starts with a #
                else
                {
                    // the line is a comment
                    if (debug)
                    {
                        cout << "Comment detected: " << line << endl;
                    }
                }
            }
        }
    }

    // close file
    file.close();
    
    // print the memory state
    vm.print_memory_state();
    
    return 0;
}