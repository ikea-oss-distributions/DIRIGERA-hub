#!/usr/bin/env python
"""
Checkpatch analyse
~~~~~~~~~~ ~~~~~~~
"""

import logging
import sys
import os
import fnmatch

def _getenv_list(key, default=None, sep=':'):
    """
    'PATH' => ['/bin', '/usr/bin', ...]
    """
    value = os.getenv(key)
    if value is None:
        return default
    else:
        return value.split(sep)

CHECKPATCH_IGNORED_FILES = _getenv_list('CHECKPATCH_IGNORED_FILES', [
        '.gitignore'])

CHECKPATCH_IGNORED_KINDS = _getenv_list('CHECKPATCH_IGNORED_KINDS', [
        'GERRIT_CHANGE_ID',
        'CONFIG_DESCRIPTION',
        'UNNECESSARY_PARENTHESES',
        'FILE_PATH_CHANGES'])

CHECKPATCH_IGNORED_KINDS_DTS = _getenv_list('CHECKPATCH_IGNORED_KINDS_DTS', [
        'GERRIT_CHANGE_ID',
        'LONG_LINE',
        'LONG_LINE_COMMENT',
        'FILE_PATH_CHANGES'])

STYLE_LINK = os.getenv('STYLE_LINK',
        'http://intranet.lme.st.com:8000/php-bin/mm_wiki/index.php/Kernel_device_coding_guidelines')

def parse_checkpatch_output(out, path_line_comments, warning_count):
    """
    Parse string output out of CHECKPATCH into path_line_comments.
    Increment warning_count[0] for each warning.

    path_line_comments is { PATH: { LINE: [COMMENT, ...] }, ... }.
    """
    def add_comment(path, line, level, kind, message):
        """_"""
        logging.debug("add_comment %s %d %s %s '%s'",
                      path, line, level, kind, message)

        if fnmatch.fnmatch(path, '*.dts*'):
            if kind in CHECKPATCH_IGNORED_KINDS_DTS:
                return
        elif kind in CHECKPATCH_IGNORED_KINDS:
            return

        for pattern in CHECKPATCH_IGNORED_FILES:
            if fnmatch.fnmatch(path, pattern):
                return

        path_comments = path_line_comments.setdefault(path, {})
        line_comments = path_comments.setdefault(line, [])
        line_comments.append('(checkpatch) ' + message)
        warning_count[0] += 1

    level = None # 'ERROR', 'WARNING', CHECK
    kind = None # 'CODE_INDENT', 'LEADING_SPACE', ...
    message = None # 'code indent should use tabs where possible'

    for line in out:
        # ERROR:CODE_INDENT: code indent should use tabs where possible
        # #404: FILE: lustre/liblustre/dir.c:103:
        # +        op_data.op_hash_offset = hash_x_index(page->index, 0);$
        line = line.strip()
        if not line:
            level, kind, message = None, None, None
        elif line[0] == '#':
            # '#404: FILE: lustre/liblustre/dir.c:103:'
            tokens = line.split(':', 5)
            if len(tokens) != 5 or tokens[1] != ' FILE':
                continue

            path = tokens[2].strip()
            line_number_str = tokens[3].strip()
            if not line_number_str.isdigit():
                continue

            line_number = int(line_number_str)

            if path and level and kind and message:
                add_comment(path, line_number, level, kind, message)
        elif line[0] == '+':
            continue
        else:
            # ERROR:CODE_INDENT: code indent should use tabs where possible
            try:
                level, kind, message = line.split(':', 2)
            except ValueError:
                level, kind, message = None, None, None

            if level != 'ERROR' and level != 'WARNING' and level != 'CHECK':
                level, kind, message = None, None, None

def review_input_and_score(path_line_comments, warning_count):
    """
    Convert { PATH: { LINE: [COMMENT, ...] }, ... }, to a gerrit
    ReviewInput() and score
    """
    if warning_count[0] > 0:
        logging.error("%d style warning(s).\nFor more details please see %s", warning_count[0], STYLE_LINK)
        for path, line_comments in path_line_comments.iteritems():
            logging.error("\n%s", path)
            for line, comment_list in sorted(line_comments.iteritems()):
                message = '\n            '.join(comment_list)
                logging.error("line %-5s: %s", line, message)

    sys.exit(warning_count[0])


def main():
    logging.basicConfig(format='%(message)s', level=logging.ERROR)

    import sys
    out = sys.stdin.readlines()

    path_line_comments = {}
    warning_count = [0]

    parse_checkpatch_output(out, path_line_comments, warning_count)
    review_input_and_score(path_line_comments, warning_count)

if __name__ == "__main__":
    main()
