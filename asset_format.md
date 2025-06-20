# In-Memory ID
Standard IDs are 32 or 64 bit

In 32-bit id mode (the default), IDs can be represented as follows:

```
+-----------------------------------------------------------------------+
|                                                                       |
|                        32-bit unsigned integer                        |
|                                                                       |
+-----------------+-----------------------------------------------------+
|                 |                                                     |
| 8-bit domain id |                     24-bit id                       |
|                 |                                                     |
+-----------------+-----------------------------------------------------+
```

In 64-bit id mode, IDs are structured the same, except instead of the domain id being 8-bit, the domain-id is 16-bit


```
+--------------------------------------------------------------------------+
|                                                                          |
|                          32-bit unsigned integer                         |
|                                                                          |
+------------------+-------------------------------------------------------+
|                  |                                                       |
| 16-bit domain id |                      24-bit id                        |
|                  |                                                       |
+------------------+-------------------------------------------------------+
```

## ID Domains
ID domains are used during asset loading to enable fast loading without having to synchronize counters between threads (except for a couple special domain numbers).

## Static IDs
Static IDs can be set for assets. These exist in the `0xFF` or `0xFFFF` domain (depending on the id size). They can then be hardcoded safely as a constant.

## Fast ID space
This space is defined as ids with a domain of `0x00` or `0x0000`. It operates almost identically to other domains, except that it uses an atomic counter for asset ids. This is intended for use when you need to do things like load a single asset on a seperate thread, but don't want to use the standard asset load system (for example, you may have super long load time for the asset so it may not be worth making an asset loading thread wait).

### The Zero ID
Any asset marked with id `0` is considered a non-asset object. The `0` id is unique as it is perfectly valid to have any number of "assets" with the `0` id. This is useful when you need to make a runtime asset and don't want it managed by the asset system.
It is **NOT ALLOWED** to register an asset with id `0`. Doing so is undefined behavior (you can make this an error with a compilation flag).

# In-Storage Assets
The asset database is a sqlite database which uses it's own id system (either strings or uuids)

The main `assets` table has the following schema:
```sql
CREATE TABLE assets (id UUID PRIMARY KEY, path VARCHAR(255) UNIQUE NOT NULL, source_kind INTEGER NOT NULL, source_path VARCHAR(255) NOT NULL)
```

the `source_kind` column stores enum values as follows


| enum | description     |
|------|-----------------|
| `0`  | normal file     |
| `1`  | compressed file |
| `2`  | asset pack      |


## Compressed Files
Assets can be loaded from gzip-compressed files. Denoting an asset source as compressed will make the system preprocess the source by loading the compressed file

# Asset Packs
The best way to distribute assets is to package them into a single binary file.
Asset packs also can reduce overhead by reducing the number of system interrupts. This is done because asset packs can be loaded all at once instead of having to read dozens of files.


> # Implementation Status
> Right now, the database isn't implemented, and neither is almost anything.
> The first thing to work on is runtime asset loading and the id system.
> Later, I will add the asset database (and likely write a python tool for building asset databases and asset packs)


# Asset Metadata
(Originally unplanned and not yet implemented) All assets will have some way to store metadata like unique name (default to path), static id (default to none), and loader (required).
This will allow me to update game content without having to recompile everything.

This will work by making all assets which are stored in an external format loaded using an asset loader derived from one of the multifile loaders (either the base multifile loader which allows for several files if that is necessary, or the more specialized file+json loader).

A generic loader will be used for initial asset loading, which will then get and pass off to the correct asset loader.

## Loading
Assets can be loaded either by source name (for file+meta assets this might make more sense, especially since the loader will set the name to the source path by default, not the meta path) or by metadata file (which will always work).
The underlying logic will check if there is a meta file for the asset you are trying to load, and if there is it'll load from that. Otherwise it'll treat the file you are asking it to load as if it's a metadata file. You will recieve an error if the file deduced to be metadata fails to load.


# A weird note about the asset implementation
The asset loading system supports a user-provided options field, however this is currently unused.

This options type will likely remain as `std::nullptr_t` for standard usage. The main purpose is to allow for certain kinds of loaders to work (for example, you might use this if you have a shader embedded in the executable so you can provide the correct information).

The current asset manager system does not allow registered loaders with options types other than `std::nullptr_t`, however you can tell it to load non-generic in order to get the desired logic.
