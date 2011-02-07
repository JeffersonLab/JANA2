

PACKAGES := gSOAP

# Explicitly add these since they won't necessarily exist if soapcpp2 hasn't been run yet
CXXSRC += soapC.cpp soapServer.cpp

# Tell make to create soapH.h first which is needed by jcalibws.cc
erste: soapH.h all

ifndef BMSDIR
BMSDIR := ../../../BMS
endif

include $(BMSDIR)/Makefile.bin

# Rule for generating gSOAP source from jcalibws.h header
soapC.cpp soapServer.cpp soapH.h: jcalibws.h
	$(SOAPCPP2) -S -I$(GSOAP)/share/gsoap/import $<
	rm -f soapServerLib.cpp

# Override the default clean so we can get rid of all of the files soapcpp2 generates
clean:
	rm -f .obj/$(BMS_OSNAME)/*.o *~
	rm -f jcalibws.GetKeyValue.req.xml    jcalibws.GetTable.req.xml
	rm -f jcalibws.GetKeyValue.res.xml    jcalibws.GetTable.res.xml
	rm -f jcalibws.nsmap jcalibws.wsdl ns.xsd soapC.cpp soapH.h soapStub.h
	rm -f soapjcalibwsProxy.h soapjcalibwsObject.h
	rm -f soapServer.cpp soapServerLib.cpp 
	rm -f soapClient.cpp soapClientLib.cpp 

	
