
              +-----------------------------------------------------------+
              |                                                           |
              |          transient_binding                                |
              |                 ^                                         |
              |                 |                                         |
              |       +---->transient<>-->dispatcher<>-->factory<>-->arg--+
              |       |                                     ^
              |       |         +-->scope------+-->global   T
              |       |         |     ^        |            |
              |       |         |     | parent |            |
              |       |         |     | scope  |            |
              v       |         |     |        |            |
          composer<>--+         |     +--------+            |
              ^       |         |                +----------+---------+
              |       +----->shared------+       |          |         |
              |                 |        |    static     direct    external
              |                 v        |                           < >
              |          shared_binding  |                            |
              |                          |                            v
              +--------------------------+                     resolved_factory

