# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.
#
# This is a simple example JANA python script. It shows how to add plugins
# and set configuration parameters. Event processing will start once this
# script exits.
import jana

jana.AddPlugin('JTest')
jana.SetParameterValue('jana:nevents', 200)

jana.Run()
