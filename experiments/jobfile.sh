#!/bin/bash
#
#
#
# Traces:
#    605.mcf_s-472B
#    607.cactuBSSN_s-2421B
#    619.lbm_s-2677B
#    620.omnetpp_s-141B
#    621.wrf_s-6673B
#    623.xalancbmk_s-10B
#    628.pop2_s-17B
#    649.fotonik3d_s-10881B
#    654.roms_s-1007B
#    cassandra_phase0_core0
#    cloud9_phase5_core2
#    nutch_phase0_core0
#    streaming_phase0_core1
#
#
# Experiments:
#    pythia: --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=$(PYTHIA_HOME)/config/pythia.ini
#
#
#
#
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 605.mcf_s-472B -traces /home/Pythia/traces/605.mcf_s-472B.champsimtrace.xz > 605.mcf_s-472B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 607.cactuBSSN_s-2421B -traces $PYTHIA_HOME/traces/607.cactuBSSN_s-2421B.champsimtrace.xz > 607.cactuBSSN_s-2421B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 619.lbm_s-2677B -traces /home/Pythia/traces/619.lbm_s-2677B.champsimtrace.xz > 619.lbm_s-2677B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 620.omnetpp_s-141B -traces /home/Pythia/traces/620.omnetpp_s-141B.champsimtrace.xz > 620.omnetpp_s-141B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 621.wrf_s-6673B -traces $PYTHIA_HOME/traces/621.wrf_s-6673B.champsimtrace.xz > 621.wrf_s-6673B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 623.xalancbmk_s-10B -traces $PYTHIA_HOME/traces/623.xalancbmk_s-10B.champsimtrace.xz > 623.xalancbmk_s-10B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 628.pop2_s-17B -traces /home/Pythia/traces/628.pop2_s-17B.champsimtrace.xz > 628.pop2_s-17B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 649.fotonik3d_s-10881B -traces $PYTHIA_HOME/traces/649.fotonik3d_s-10881B.champsimtrace.xz > 649.fotonik3d_s-10881B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini  --traceName 654.roms_s-1007B -traces $PYTHIA_HOME/traces/654.roms_s-1007B.champsimtrace.xz > 654.roms_s-1007B_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini --knob_cloudsuite=true --warmup_instructions=100000000 --simulation_instructions=150000000 --traceName cassandra_phase0_core0 -traces /home/Pythia/traces/cassandra_phase0_core0.trace.xz > cassandra_phase0_core0_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini --knob_cloudsuite=true --warmup_instructions=100000000 --simulation_instructions=150000000 --traceName cloud9_phase5_core2 -traces /home/Pythia/traces/cloud9_phase5_core2.trace.xz > cloud9_phase5_core2_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini --knob_cloudsuite=true --warmup_instructions=100000000 --simulation_instructions=150000000 --traceName nutch_phase0_core0 -traces /home/Pythia/traces/nutch_phase0_core0.trace.xz > nutch_phase0_core0_pythia.out 2>&1
/home/Pythia/bin/perceptron-multi-multi-no-ship-1core --warmup_instructions=100000000 --simulation_instructions=500000000 --l2c_prefetcher_types=scooby --config=/home/Pythia/config/pythia.ini --knob_cloudsuite=true --warmup_instructions=100000000 --simulation_instructions=150000000 --traceName streaming_phase0_core1 -traces /home/Pythia/traces/streaming_phase0_core1.trace.xz > streaming_phase0_core1_pythia.out 2>&1
