# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cache-hit) begin
(cache-hit) create "test"
(cache-hit) open "test"
(cache-hit) seek "test" to 0
(cache-hit) reading "test"
(cache-hit) closing "test"
(cache-hit) reopening "test"
(cache-hit) reading "test"
(cache-hit) should have less cache misses on 2nd read
(cache-hit) end
cache-hit: exit(0)
EOF
pass;