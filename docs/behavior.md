
# Behavior specification

## Introduction

## Initialization
If no `JEventSources` are present in the processing topology, `JApplication::Initialize()` will still succeed, and all present components will be initialized.

## Execution
If no `JEventSources` are present in the processing topology, `JExecutionEngine::Run()` will immediately throw a detailed `JException`. This will terminate `JApplication::Run()` and cause `JMain::Execute()` to exit the program.


## Finalization



