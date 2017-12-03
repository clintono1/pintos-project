# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cache-hit) begin
(cache-hit) create "test"
(cache-hit) open "test"
cache-hit: exit(-1)
EOF
pass;