# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(coalesce) begin
(coalesce) create "test"
(coalesce) open "test"
(coalesce) seek "test" to 0
(coalesce) number of disk writes: "128"
(coalesce) end
EOF
pass;