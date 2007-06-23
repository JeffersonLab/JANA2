#!/bin/tcsh

set old=`basename $1 .cc`
set new=`echo $old | sed -e s/^D/J/`

cat $1 | \
  sed -e s/DApplication/JApplication/g | \
  sed -e s/DEvent/JEvent/g | \
  sed -e s/DEventLoop/JEventLoop/g | \
  sed -e s/DEventProcessor/JEventProcessor/g | \
  sed -e s/DEventSink/JEventSink/g | \
  sed -e s/DEventSource/JEventSource/g | \
  sed -e s/DEventSourceHDDM/JEventSourceHDDM/g | \
  sed -e s/DEventSourceJIL/JEventSourceJIL/g | \
  sed -e s/DException/JException/g | \
  sed -e s/DFactory_base/JFactory_base/g | \
  sed -e s/DGeometry/JGeometry/g | \
  sed -e s/DLog/JLog/g | \
  sed -e s/DParameter/JParameter/g | \
  sed -e s/DParameterManager/JParameterManager/g | \
  sed -e s/DStreamLogBuffer/JStreamLogBuffer/g | \
  sed -e s/DStreamLog/JStreamLog/g | \
  sed -e s/derror/jerror/g \
  \
  > ${new}.cc
