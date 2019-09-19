import sys
import re

regexes = [

    # JFactories

    (re.compile(r"jana::JFactory<([a-zA-Z_0-9]+)>"), r'JFactoryT<\1>'),

    (re.compile(r'#include <JANA/JFactory.h>'), r'#include <JANA/JFactoryT.h>'),

    (re.compile(r'jerror_t init\(void\);						///< Called once at program start.'),
     r'void Init() override;'),

    (re.compile(r'jerror_t brun\(jana::JEventLoop \*eventLoop, int32_t runnumber\);	///< Called everytime a new run number is detected.'),
     r'void ChangeRun(const std::shared_ptr<const JEvent>& event) override;'),

    (re.compile(r'jerror_t evnt\(jana::JEventLoop \*eventLoop, uint64_t eventnumber\);	///< Called every event.'),
     r'void Process(const std::shared_ptr<const JEvent>& aEvent) override;'),

    (re.compile(
        r'jerror_t erun\(void\);						///< Called everytime run number changes, provided brun has been called.'),
     r'void EndRun();'),

    (re.compile(r'jerror_t fini\(void\);						///< Called after last event of last event source has been processed.'),
     r'void Finish();'),

    (re.compile(r'jerror_t ([a-zA-Z_0-9]+)::init\(void\)'),
     r'void \1::Init()'),

    (re.compile(r'jerror_t ([a-zA-Z_0-9]+)::brun\(JEventLoop \*eventLoop, int32_t runnumber\)'),
     r'void \1::ChangeRun(const std::shared_ptr<const JEvent>& event)'),

    (re.compile(r'jerror_t ([a-zA-Z_0-9]+)::evnt\(JEventLoop \*loop, uint64_t eventnumber\)'),
     r'void \1::Process(const std::shared_ptr<const JEvent>& event)'),

    (re.compile(r'jerror_t ([a-zA-Z_0-9]+)::erun\(void\)'),
     r'void \1::EndRun()'),

    (re.compile(r'jerror_t ([a-zA-Z_0-9]+)::fini\(void\)'),
     r'void \1::Finish()'),

    # JFactoryGenerators

    (re.compile(r'jana::'), ''),

    (re.compile(r'#include <JANA/jerror\.h>\n'), ''),

    (re.compile(r'jerror_t GenerateFactories\(JEventLoop \*loop\)'),
     r'void GenerateFactories(JFactorySet *factorySet)'),

    (re.compile(r'loop->AddFactory'),
     r'factorySet->Add'),

    (re.compile(r'jout((?:(?! endl).)*) endl;'),
     r'jout\1 jendl;'),

    (re.compile(r'jout((?:(?!>endl).)*)>endl;'),
     r'jout\1>jendl;')
]


def main():
    filename = sys.argv[1]
    with open(filename, 'r') as f:
        contents = f.read()

    for (before, after) in regexes:
        contents = re.sub(before, after, contents)

    with open(filename, 'w') as f:
        f.write(contents)
        print(contents)


if __name__ == '__main__':
    main()
