# CS230_project
<p align="center">
    <h1 align="center"> L-TAGE Branch Predictor</h1>
    L-TAGE is a PPM based branch predictor, a hybrid of the TAGE branch predictor and loop predictor Here, we present an implementation of L-TAGE and TAGE branch predictors using the Champsim simulator.
</p>

# Instructions to run
- Clone the repository
```
$ git clone https://github.com/Hanvitha-Reddy/CS230_project
``` 
- Download the traces from [here](https://drive.google.com/file/d/1qs8t8-YWc7lLoYbjbH_d3lf1xdoYBznf/view?usp=sharing) and extract them into a folder

- You might want to follow instructions of the Champsim repository [here](https://github.com/ChampSim/ChampSim)

- Execute the script ***run.sh*** to generate logs of all the traces for the specified branch predictor
```
# Usage : ./run.sh [-b BUILD] [-r RUN] [trace_dir] [branch_predictor] [binary_name OPTIONAL] [results_directory OPTIONAL]
# example: 
$ ./run.sh -br ../traces ltage ltage_binary ltage_results 
```

- The results will be created for each trace in the results directory specified.
- To compare results from two branch predictors, execute the following command


# Code Structure

Since the implementation is made specific to Champsim simulator, the code structure is the same as the source code of Champsim.   

Below is the description of files containing the implementation of the branch predictors.

| File                                          | Description                                                                           |
| ---                                           | ---                                                                                   |                         
| [branch/loop_pred.h](branch/loop_pred.h)      | It implements loop predictor                                                          |
| [branch/ltage.bpred](branch/ltage.bpred)      | It implements the Champsim functions associated with branch prediction for LTAGE      |
| [branch/Tage.bpred](branch/Tage.bpred)        | It implements the Champsim functions associated with branch prediction for TAGE       |
| [branch/tage.h](branch/tage.h)                | It implements TAGE predictor                                                          |

Below is the description of folders containing the results of the branch predictors.

| Folder                                        | Description                                                                           |
| ---                                           | ---                                                                                   | 
| [ltage](ltage)                                | contains results associated with ltage predictor for each server trace file.          |
| [tage](tage)                                  | contains results associated with tage predictor for each server trace file.           |
| [hashed_perceptron](hashed_perceptron)        | contains results associated with hashed perceptron predictor for each server trace file|
| [results](results)                            | contains performance scores of each predictor                                         |

