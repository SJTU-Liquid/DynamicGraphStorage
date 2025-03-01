### LDBC dataset

1. Download and generate ldbc dataset from [ldbc_snb_datagen_hadoop](https://github.com/ldbc/ldbc_snb_datagen_hadoop/).

2. Synthesize ldbc dataset to a uniformed graph dataset for workload generation

   1. synthesize initial snapshot

      ```bash
      ./initial_snapshot.sh /path/to/ldbc/home /path/to/a/tmp/storage/dir /path/to/output/dir
      ```

   2. synthesize update stream

      ```bash
      ./update_stream.sh /path/to/ldbc/home /path/to/output/dir
      ```

      

