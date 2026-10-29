# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
import unittest
from compare_shadow import compare

class ShadowComparisonTest(unittest.TestCase):
    def test_numeric_format_is_not_a_difference(self):
        self.assertEqual(compare(['assign 1.000', 'move 1.0 2 3 8 default'], ['assign 1', 'move 1 2 3 8 default'], .01), (None, 0))

    def test_small_destination_roundoff_is_allowed(self):
        reason, error = compare(['move 1.0001 2 3 8 default'], ['move 1 2 3 8 default'], .01)
        self.assertIsNone(reason)
        self.assertGreater(error, 0)

    def test_different_action_is_not_hidden_by_tolerance(self):
        self.assertEqual(compare(['stop'], ['finish'], 100)[0], 'action_kind')

    def test_different_assignment_result_is_never_tolerated(self):
        self.assertEqual(compare(['assign 0'], ['assign 1'], 100)[0], 'assignment_result')

    def test_wrong_target_or_flags_are_never_tolerated(self):
        for wrong in ['assignUnit 42 1 0 1', 'assignUnit 41 0 0 1', 'assignUnit 41 1 1 1']:
            self.assertEqual(compare([wrong], ['assignUnit 41 1 0 1'], 100)[0], 'assignment_result')

    def test_large_destination_error_fails(self):
        self.assertEqual(compare(['move 1 2 3 8 default'], ['move 2 2 3 8 default'], .01)[0], 'destination')

    def test_nonfinite_destination_fails(self):
        self.assertEqual(compare(['move nan 2 3 8 default'], ['move 1 2 3 8 default'], .01)[0], 'nonfinite')

    def test_missing_action_fails(self):
        self.assertEqual(compare([], ['stop'], .01)[0], 'action_count')

if __name__ == '__main__': unittest.main()
