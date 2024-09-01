

add_jana_plugin(CsvWriter)

add_test(
    NAME jana-example-csv-writer
    COMMAND jana -Pjana:nevents=5 -Pplugins=JTest,CsvWriterExample -Pcsv:collection_names=sadf:sfd)
