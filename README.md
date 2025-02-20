# VirtualMemorySimulator

A program from my operating systems class that simulates virtual memory being written to and read from.

## Description

This program simulates virtual memory by reading in a read or write command with a page number and storing the information in a frame and page table.

Each frame knows if the page number that is currently in that frame, if the frame is in use, if the frame is dirty, and the first and last use of the frame. Each page knows it's page number, the frame number the page is stored in, if the page is on disk, and if the page is unused, stolen, or mapped.

Three page replacement algorithms are implements, First In First Out (FIFO), Least Recently Used (LRU), and Optimal (OPT). When the program is run, the algorithm that will be used is set by typing it into the command line arguement (example: `./vm LRU input.0.psize1`)

## Folder Structure

All files needed to run the program are not stored in folders except the following:

- `correct_answers`: the folder stores all the correct answers for each of the input files using each algorithm
- `results`: a folder that is created when the the command `make test` is ran

## Execution

The MakeFile will compile the `vm.cc` file. The following commands can be ran with the MakeFile include:

- `make`: compiles `vm.cc` into an executable called `vm`
- `make test`: compiles `vm.cc` and runs each of the tests with each of the algorithms and outputs whether they match the correct answer file in `correct_answers` or not into the terminal and the `.test.results` file
- `make clean`: removes the `results` folder, `vm` executable, all .o files, and the `.test.result` file

Additionally, each of the test files can be run individually with the following commands:

- `vm` can be ran in the terminal with any of the algorithms and input files (example: `./vm FIFO input.w.bs`) 
- `./test` can be ran with any of the input files to check if the output of FIFO and LRU matches the correct answers (example: `./test input.w.bs)
- `./testoptimal` can be ran to check if the output of the OPTIMAL algorithm matches the correct answer for all the input files (example: `./testoptimal input.w.bs`)

## Credit

All files test files, correct answers, and the `Makefile` were made by Shawn Ostermann. They are there to for future use if the `vm.cc` needs to be reran. All of `vm.cc` was made by me.