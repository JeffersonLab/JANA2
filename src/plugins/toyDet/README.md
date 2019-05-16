
## Streaming Toy Detector Plugin

### Overview

The toy detector (toyDet) plugin is aimed at analyzing simulated streaming data.  Both ADC and TDC data are simulated by the `toyDet.py` script which produces a flat text (data) file.  The `toyDet` JANA2 plugin reads in this file and produces a ROOT file `outFile.root`.  This ROOT file contains leaves with the corresponding ADC and TDC data.

### toyDet.py 

The data are assumed to eminate from a SAMPA chips which are comprised of 1024 ADC and TDC samples per readout window.  Each readout has been definied as an "event".  The simulated TDC data is a continuous stream of sampled TDC pulses whose frequency depends on the sampling rate.  The ADC data is a sampled analog signal whose shape is governed by a Gumbel distribution whose parameters are randomized from pulse to pulse.  Each sample of the ADC data is directly correlated with the continuous stream of TDC pulses ergo there exists a one to one mapping between the sampled data.

- Usage

```
python toyDet.py sampleRate numChans numEvents
```
- sampleRate (int) is the sampling rate in MHz of the SAMPA chips (5, 10, or 20 MHz)
- numChans   (int) is the number of channels to simulate ADC and TDC data for
- numEvents  (int) is the number of events to simulate for each channel

### toyDet plugin

This JANA2 plugin reads in the flat text file produced via. the aforementioned python script.  These data are parsed and stored as leaves in the ROOT file `outFile.root`.

- Usage

    - To compile in $JANA_HOME/src/plugins/toyDet:

    ```
    scons install -j8
    ```

    - To execute the plugin:

    ```
    jana -Pplugins=toyDet run-X-mhz-Y-chan-Z-ev.dat -Pnthreads=N
    ```


