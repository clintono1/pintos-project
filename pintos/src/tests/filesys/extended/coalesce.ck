# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(coalesce) begin
(coalesce) create "test"
(coalesce) open "test"
(coalesce) num actual writes
(coalesce) end
EOF
pass;