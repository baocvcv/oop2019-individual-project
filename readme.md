One Pass Synthesis for DMFB

- This is an implementation of the method proposed by KesZoxze etc. (2014), in their paper called Exact One-pass Synthesis of Digital Microfluidic Biochips.

- To build this application, the following libraries are required
    - z3Prover. Installed in the default location and accessible with -lz3 for g++ compiler
    - QT5 libraries

- To build, first cd into the src directory, then run "qmake app.pro". After makefile is generated, run "make" to build the application and use "./app" to start the app.
