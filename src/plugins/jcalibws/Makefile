
SOLIB_NAME = jcalibws.so

PACKAGES := GSOAP

# Explicitly add these since they won't necessarily exist if soapcpp2 hasn't been run yet
CXXSRC += soapC.cpp soapClient.cpp

# Tell make to create soapH.h first which is needed by jcalibws.cc
erste: soapH.h all

BMSDIR := ../../BMS
include $(BMSDIR)/Makefile.shlib

# Rule for generating gSOAP source from jcalibws.h header
soapC.cpp soapClient.cpp soapH.h: jcalibws.h
	$(SOAPCPP2) -C -I$(GSOAP)/share/gsoap/import $<
	rm -f soapClientLib.cpp

# Override the default clean so we can get rid of all of the files soapcpp2 generates
clean:
	rm -f .obj/$(BMS_OSNAME)/*.o *~
	rm -f jcalibws.GetKeyValue.req.xml    jcalibws.GetTable.req.xml
	rm -f jcalibws.GetKeyValue.res.xml    jcalibws.GetTable.res.xml
	rm -f jcalibws.nsmap jcalibws.wsdl ns.xsd soapC.cpp soapH.h soapStub.h
	rm -f soapjcalibwsProxy.h soapjcalibwsObject.h
	rm -f soapServer.cpp soapServerLib.cpp 
	rm -f soapClient.cpp soapClientLib.cpp 

