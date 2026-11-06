# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
import unittest
from pathlib import Path
from replays import compare, samples


class ParityChecks(unittest.TestCase):
    def test_requires_every_sample_and_completion(self):
        for frames, complete in [({}, True), ({0: ['ab', 'cd']}, True),
                                 ({0: ['ab', 'cd'], 150: ['ab', 'cd']}, False)]:
            reader = lambda _: ({}, frames, complete)
            self.assertFalse(samples(Path('.'), 150, reader)[0])

    def test_detects_events_and_states(self):
        rows = {'a': {0: ['ab', 'cd'], 150: ['ab', 'cd']},
                'b': {0: ['ab', 'cd'], 150: ['ab', 'ef']}}
        reader = lambda directory: ({}, rows[directory], True)
        result = compare('a', 'b', 150, reader)
        self.assertFalse(result['equal'])
        self.assertEqual(result['first_difference_frame'], 150)
        self.assertTrue(compare('a', 'a', 150, reader)['equal'])

    def test_missing_artifact_is_incomplete(self):
        def reader(_):
            raise FileNotFoundError()
        self.assertFalse(compare('a', 'b', 150, reader)['equal'])


if __name__ == '__main__':
    unittest.main()
