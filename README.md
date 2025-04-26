<p align="center">
  <h3 align="center">Context-Aware Dynamically Weighted Prefetcher
  </h3>
</p>
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
	<li><a href="#what-is-Context-Aware-Dynamically-Weighted-Prefetcher">What is Context-Aware Dynamically Weighted Prefetcher?</a></li>
    <li><a href="#prerequisites">Prerequisites</a></li>
    <li><a href="#installation">Installation</a></li>
    <li><a href="#preparing-traces">Preparing Traces</a></li>
    <ul>
      <li><a href="#more-traces">More Traces</a></li>
    </ul>
    <li><a href="#experimental-workflow">Experimental Workflow</a></li>
      <ul>
        <li><a href="#launching-experiments">Launching Experiments</a></li>
        <li><a href="#rolling-up-statistics">Rolling up Statistics</a></li>
      </ul>
    </li>
    <li><a href="#hdl-implementation">HDL Implementation</a></li>
    <li><a href="#code-walkthrough">Code Walkthrough</a></li>
    <li><a href="#citation">Our Citation</a></li>
  </ol>
</details>

## What is Context-Aware Dynamically Weighted Prefetcher?

> Pythia is a hardware-realizable, light-weight data prefetcher that uses reinforcement learning to generate accurate, timely, and system-aware prefetch requests. 
In modern computing systems, the widening gap between processor speed and memory access latency creates a critical performance bottleneck, with hardware prefetching emerging as an essential technique to hide memory latency. While recent machine learning-based prefetchers like Pythia have shown promise by leveraging reinforcement learning (RL), they often struggle with dynamic workloads exhibiting irregular access patterns or phase transitions.
Context-Aware Dynamically Weighted Prefetcher addresses these challenges, Our design introduces a two-level error tracking mechanism to detect execution phase changes and employs context vector matching to dynamically switch between stored weight sets, enabling rapid adaptation without retraining from scratch.

## Prerequisites

The infrastructure has been tested with the following system configuration:
  * G++ v6.3.0 20170516
  * CMake v3.20.2
  * md5sum v8.26
  * Perl v5.24.1

## Installation

0. Install necessary prequisites
    ```bash
    sudo apt install perl
    ```
1. Clone the GitHub repo
   
   ```bash
   git clone https://github.com/CMU-SAFARI/Pythia.git
   ```
2. Clone the bloomfilter library inside Pythia home directory
   
   ```bash
   cd Pythia
   git clone https://github.com/mavam/libbf.git libbf
   ```
3. Build bloomfilter library. This should create the static `libbf.a` library inside `build` directory
   
    ```bash
    cd libbf
    mkdir build && cd build
    cmake ../
    make clean && make
    ```
4. Build Pythia for single/multi core using build script. This should create the executable inside `bin` directory.
   
   ```bash
   cd $PYTHIA_HOME
   # ./build_champsim.sh <l1_pref> <l2_pref> <llc_pref> <ncores>
   ./build_champsim.sh multi multi no 1
   ```
   Please use `build_champsim_highcore.sh` to build ChampSim for more than four cores.

5. _Set appropriate environment variables as follows:_

    ```bash
    source setvars.sh
    ```

## Preparing Traces

> [Update: Dec 18, 2024] The trace will be downloaded in two phases: (1) all traces, except Ligra and PARSEC workloads, will be downloaded using the automated script, and (2) the Ligra and PARSEC traces needs to be downloaded manually from Zenodo repository mentioned below. 

1. Use the `download_traces.pl` perl script to download all traces, except Ligra and PARSEC.

    ```bash
    mkdir $PYTHIA_HOME/traces/
    cd $PYTHIA_HOME/scripts/
    perl download_traces.pl --csv artifact_traces.csv --dir ../traces/
    ```
> Note: The script should download **138** traces. Please check the final log for any incomplete downloads.

2. Once the trace download completes, please verify the checksum as follows. _Please make sure all traces pass the checksum test._

    ```bash
    cd $PYTHIA_HOME/traces
    md5sum -c ../scripts/artifact_traces.md5
    ```

3. Download the Ligra and PARSEC traces from these repositories:
    - Ligra: https://doi.org/10.5281/zenodo.14267977
    - PARSEC 2.1: https://doi.org/10.5281/zenodo.14268118

4. If the traces are downloaded in some other path, please change the full path in `experiments/MICRO21_1C.tlist` and `experiments/MICRO21_4C.tlist` accordingly.

### Traces for this project
* 605.mcf_s-472B.champsimtrace.xz
* 607.cactuBSSN_s-2421B.champsimtrace.xz
* 619.lbm_s-2677B.champsimtrace.xz
* 620.omnetpp_s-141B.champsimtrace.xz
* 621.wrf_s-6673B.champsimtrace.xz
* 623.xalancbmk_s-10B.champsimtrace.xz
* 628.pop2_s-17B.champsimtrace.xz
* 649.fotonik3d_s-10881B.champsimtrace.xz
* 654.roms_s-1007B.champsimtrace.xz
* cassandra_phase0_core0.trace.xz
* cloud9_phase5_core2.trace.xz
* nutch_phase0_core0.trace.xz
* streaming_phase0_core1.trace.xz


## Experimental Workflow
Our experimental workflow consists of two stages: (1) launching experiments, and (2) rolling up statistics from experiment outputs.

### Launching Experiments
1. To create necessary experiment commands in bulk, we will use `scripts/create_jobfile.pl`
2. `create_jobfile.pl` requires three necessary arguments:
      * `exe`: the full path of the executable to run
      * `tlist`: contains trace definitions
      * `exp`: contains knobs of the experiements to run
3. Create experiments as follows. _Please make sure the paths used in tlist and exp files are appropriate_.
   
      ```bash   
	        cd $PYTHIA_HOME/experiments/
			export PYTHIA_HOME=/home/Pythia
			perl -I../scripts ../scripts/create_jobfile.pl \
			  --exe $PYTHIA_HOME/bin/perceptron-multi-multi-no-ship-1core \
			  --tlist my_traces.tlist \
			  --exp MICRO21_1C_2.exp \
			  --local 1 \
			  > jobfile.sh
      ```

4. Go to a run directory (or create one) inside `experiements` to launch runs in the following way:
      ```bash
        cd experiments_1C
		export PYTHIA_HOME=/home/Pythia
		source ../jobfile.sh
      ```

5. If you have [slurm](https://slurm.schedmd.com) support to launch multiple jobs in a compute cluster, please provide `--local 0` to `create_jobfile.pl`

### Rolling-up Statistics
1. To rollup stats in bulk, we will use `scripts/rollup.pl`
2. `rollup.pl` requires three necessary arguments:
      * `tlist`
      * `exp`
      * `mfile`: specifies stat names and reduction method to rollup
3. Rollup statistics as follows. _Please make sure the paths used in tlist and exp files are appropriate_.
   
      ```bash
		export PYTHIA_HOME=/home/Pythia
		perl ../../scripts/rollup.pl --tlist ../my_traces.tlist  --exp ../MICRO21_1C_2.exp --mfile ../rollup_1C_base_config.mfile > rollup.csv

      ```

4. Export the `rollup.csv` file in you favourite data processor (Python Pandas, Excel, Numbers, etc.) to gain insights.

## HDL Implementation
We also implement Pythia in [Chisel HDL](https://www.chisel-lang.org) to faithfully measure the area and power cost. The implementation, along with the reports from umcL65 library, can be found the following GitHub repo. Please note that the area and power projections in the sample report is different than what is reported in the paper due to different technology.

<p align="center">
<a href="https://github.com/CMU-SAFARI/Pythia-HDL">Pythia-HDL</a>
    <a href="https://github.com/CMU-SAFARI/Pythia-HDL">
        <img alt="Build" src="https://github.com/CMU-SAFARI/Pythia-HDL/actions/workflows/test.yml/badge.svg">
    </a>
</p>

## Code Walkthrough
> Pythia was code-named Scooby (the mistery-solving dog) during the developement. So any mention of Scooby anywhere in the code inadvertently means Pythia.

* The top-level files for Pythia are `prefetchers/scooby.cc` and `inc/scooby.h`. These two files declare and define the high-level functions for Pythia (e.g., `invoke_prefetcher`, `register_fill`, etc.). 
* The released version of Pythia has two types of RL engine defined: _basic_ and _featurewise_. They differ only in terms of the QVStore organization (please refer to our [paper](arxiv.org/pdf/2109.12021.pdf) to know more about QVStore). The QVStore for _basic_ version is simply defined as a two-dimensional table, whereas the _featurewise_ version defines it as a hierarchichal organization of multiple small tables. The implementation of respective engines can be found in `src/` and `inc/` directories.
* `inc/feature_knowledge.h` and `src/feature_knowldege.cc` define how to compute each program feature from the raw attributes of a deamand request. If you want to define your own feature, extend the enum `FeatureType` in `inc/feature_knowledge.h` and define its corresponding `process` function.
* `inc/util.h` and `src/util.cc` contain all hashing functions used in our evaluation. Play around with them, as a better hash function can also provide performance benefits.

## Our Citation
We finished this project mainly base on this paper 
```
@inproceedings{bera2021,
  author = {Bera, Rahul and Kanellopoulos, Konstantinos and Nori, Anant V. and Shahroodi, Taha and Subramoney, Sreenivas and Mutlu, Onur},
  title = {{Pythia: A Customizable Hardware Prefetching Framework Using Online Reinforcement Learning}},
  booktitle = {Proceedings of the 54th Annual IEEE/ACM International Symposium on Microarchitecture},
  year = {2021}
}
```

Also based on the code work of the pythia authors
```
https://github.com/CMU-SAFARI/Pythia
```
