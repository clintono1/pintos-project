# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(coalesce-writes) begin
coalesce-writes: exit(-1)
EOF
pass;