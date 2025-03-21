# Parameter Changes

This guide outlines the key parameter changes from JANA1 to JANA2, helping you adjust your configurations and usage as needed.

### Loading Configuration

- **JANA1:** hd_root `--config=my_file.config` input.evio
- **JANA2:** hd_root `--loadconfigs my_file.config` input.evio

### Commonly Used JANA Parameters

The following table compares commonly used JANA parameters between JANA1 and JANA2. Parameters that remain unchanged in JANA2 are marked with `-`. In the "Possible Input" column, `-` means that they are the same as in JANA1. Changes are listed where applicable.

| **Parameter**         | **JANA1**              | **JANA2**                    | **Possible Inputs**                              |
|-----------------------|------------------------|------------------------------|--------------------------------------------------|
| **PLUGINS**           | `- `                   | `-`                          | `-`                                              |
| **EVENTS_TO_KEEP**    | `EVENTS_TO_KEEP`       | `jana:nevents`               | `-`                                              |
| **EVENTS_TO_SKIP**    | `EVENTS_TO_SKIP`       | `jana:nskip`                | `-`                                              |
| **NTHREADS**          | `-`                    | `-`                          | `-`                                              |
| **JANA:BATCH_MODE**   | `JANA:BATCH_MODE`      | `jana:global_loglevel`                 | `TRACE`, `DEBUG`, `INFO`, `WARN`, `FATAL`, `OFF` |
| **JANA_CALIB_CONTEXT**| `JANA_CALIB_CONTEXT`   | `jana:calib_context`         | `-`                                              |

## Changes in `halld_recon` Parameters

### `hd_dump` Option Update

- **JANA1:** The `-b` option was used for printing event status bits in `hd_dump`.
- **JANA2:** The `-b` option is used for benchmarking in JANA2. Thus, `hd_dump` parameter for event status bits (`-b`) has been changed to `-B`. Update your usage accordingly.

For additional assistance or questions, please contact [rasool@jlab.org](mailto:rasool@jlab.org).
