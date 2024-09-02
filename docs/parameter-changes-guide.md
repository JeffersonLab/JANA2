
# JANA1 to JANA2 Parameter Changes Guide

As part of the transition from JANA1 to JANA2, a few parameters have been updated. This guide provides a comparison of these parameters to help you adjust your configurations and usage.

## JANA Parameter Changes

| **Parameter**         | **JANA1**                        | **JANA2**                        |
|-----------------------|---------------------------------|---------------------------------|
| **PLUGINS**           | Configured with `PLUGINS`        | Updated configuration method     |
| **EVENTS_TO_KEEP**    | Previous value or syntax         | New value or syntax              |
| **EVENTS_TO_SKIP**    | Previous value or syntax         | New value or syntax              |
| **NTHREADS**          | Number of threads                | Updated usage or default value   |
| **JANA:BATCH_MODE**   | `1`                              | Updated value or usage           |
| **JANA_CALIB_CONTEXT**| Previous setting                 | Updated setting or method        |

## Changes in `halld_recon` Parameters

All parameters in `halld_recon` other than the `-b` option of `hd_dump` remain the same.

- **`hd_dump` Option Change:**
  - **JANA1:** The `-b` option was used for printing event status bits.
  - **JANA2:** The `-b` option is now used for benchmarking in JANA2. To avoid conflicts, the `hd_dump` parameter is changed to `-B`. Make sure to update your usage accordingly.

For additional assistance or if you have any questions, contact [rasool@jlab.org](mailto:rasool@jlab.org).
