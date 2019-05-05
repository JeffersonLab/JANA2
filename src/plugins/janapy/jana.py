
import janapy

janapy.AddPlugin('jtest')
janapy.SetParameterValue('NEVENTS', 10000)
janapy.Run()
janapy.PrintFinalReport()

