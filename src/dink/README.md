
              +-----------------------------------------------------------+
              |                                                           |
              |          transient_binding                                |
              |                 ^                                         |
              |                 |                                         |
              |       +---->transient<>-->dispatcher<>-->factory<>-->arg--+
              v       |                                     ^
          composer<>--+                                     T
              ^       |                          +----------+---------+
              |       +----->shared------+       |          |         |
              |                 |        |    static     direct    external
              |                 v        |                           < >
              |          shared_binding  |                            |
              |                          |                            v
              +--------------------------+                     resolved_factory

