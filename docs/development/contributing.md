Contributing
============

## Coding Conventions

- Use `std::shared_ptr` and `std::unique_ptr` internally, but don't make them be part of the API. Instead, pass a lightweight wrapper object that encapsulates the ownership model. For example, use `JComponent::Service` instead of `std::shared_ptr<JService>`.

- Use `#pragma once` instead of traditional header guards

- Methods that are part of the API are always in `PascalCase()`; methods that are not part of the contract with the user _may_ be `snake_cased`, though new code should make everything Pascal-cased. Member variables are snake-cased and prefixed with `m_`. Indent using 4 spaces, not tabs.


## Cutting a release

1. Figure out the version number. Use semver: the first number is a "major" version number that has nothing to do with the fact that this is "JANA2". 
Use release candidates when necessary. The version number convention is `v{MAJOR}.{MINOR}.{PATCH}-rc{CANDIDATE}`.

2. Update the root CMakeLists.txt to use that version number. Update the documentation at `docs/Download.md` to include a link to where the new release will be available on GitHub. Commit these changes.

3. Create a tag pointing to the commit you just made. The tag should have the same name as the CMake version, prefixed with a 'v'. Push to GitHub. Don't force push this tags, because people downstream have CI tools that cache their dependencies using the tag name instead of the hash. Instead, just cut a new release candidate.

4. Cut a release on GitHub, pointing to that tag. If it is a release candidiate, mark it as a prerelease.

5. Figure out the SHA256 of the release tarball on Github:
```bash
shasum -a 256 $PATH_TO_JANA_TARBALL
```

5. Pull-request an update to the eicrecon spack repository. The repository is here:

https://github.com/eic/eic-spack


Add a line to `packages/jana2/package.py` that associates the release version with the checksum you calculated in (5), e.g.:

```python
    version("2.2.1-rc1", sha256="7b65ce967d9c0690e22f4450733ead4acebf8fa510f792e0e4a6def14fb739b1")
```
Note that the spack version identifier does _not_ have a 'v' prefix.


