# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cache-effectiveness) begin
cache-effectiveness: exit(-1)
EOF
pass;