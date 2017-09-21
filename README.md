# Visual FlowSim: Flow-based Simulation Methodology Demonstration
This program demonstrates how flow-based simulation works. Two small examples are provided for understanding the mechanism.
Please cite the following paper:

  Hoseinzadeh, Morteza, "Flow-based Simulation Methodology," IEEE Computer Architecture Letters, Sep 2017.
  
## Requirements
  This application is built with Qt (for windows it is with MinGW). You can download an open-source version of Qt via the following link:
  
    https://www1.qt.io/download-open-source/
    
  To modify input file, we recommend using Notepad++. A highlighter file is provided in examples folder.
  
## Flow Language
  We designed a very small language to investigate different architectures, which is called Flow Language. A block is described by a pair of a string and an integer number as a unique name and a latency, respectively. The latency can be defined as random by using a `rand L H` command instead.
  A flow can be defined in 2 different types: `serial` and `concurrent`. Each block may contain only one flow. Each flow can have several blocks. There is a `conditional` block which allows us to pick a `case` with a certain probability.
  For more information, please see example files.


