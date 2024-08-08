# Use JANA2 for new experiments <!-- {docsify-ignore-all} -->

This introduction outlines common approaches to developing a flexible and efficient reconstruction system
based on JANA2 for new projects. 'new' refers to projects where software decisions 
have not yet been finalized or extensively implemented, presenting an opportunity to select 
and adapt technologies and methodologies. Many choices are very opinionated here: what data model to use, 
how geometry should be provided, how calibrations and translation is done, etc. 
This document tries to highlight some existing solutions that worked out well and could be 
used simply out of the box or close to it. 

## Examples

1. Software Infrastructure
Repository Setup: Begin by establishing a version-controlled repository, preferably on platforms like GitHub or GitLab, to manage your source code, documentation, and collaboration. This will be the central hub for your development efforts.

Continuous Integration (CI): Set up CI pipelines using tools like Jenkins, GitLab CI, or GitHub Actions. This will automate testing and building of the software, ensuring that changes are verified and do not break the existing functionality. It's crucial to integrate code quality checks and unit tests from the early stages.

Development Environment: Standardize the development environment using containerization tools like Docker or virtual environments. This ensures that all developers work within a consistent, controlled setup, reducing discrepancies between different machines and systems.

2. Designing the Architecture
Modular Design: Leverage JANA2’s modular architecture to design your software. Plan how data flows through your system from raw data input to processed outputs. Define clear interfaces for each module to facilitate future enhancements without disrupting existing functionalities.

Plugin System: Develop plugins for specific tasks such as data decoding, calibration, tracking, and event reconstruction. JANA2 supports dynamic loading of plugins, which can be developed and tested independently by different teams.

Data Management: Decide on a strategy for data storage and access, considering both performance and ease of use. Efficient handling of data is critical, given the large volumes involved in nuclear physics experiments.

3. Implementing Core Components
Event and Data Structures: Define the primary data structures that will encapsulate the raw data, intermediate states, and final reconstruction results. Ensure these structures are well-optimized for access patterns typical in nuclear physics analyses.

Concurrency Model: Exploit JANA2's ability to handle multi-threading to maximize data processing speed. Design your data structures and algorithms to be thread-safe and minimize contention points.

Testing and Validation: Develop a comprehensive suite of tests for each component. This includes unit tests for individual modules and integration tests that simulate the full data processing chain.

