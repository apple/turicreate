# -*- coding: utf-8 -*-`
# system-wide imports
import cgi
import os
import sys
import traceback

def write_declaration(output):
    output.write('<?xml version="1.0" encoding="UTF-8"?>')

def write_testcase(output, test_path, filename, exc_info):
    (type, value, tb) = exc_info
    test_name = filename.replace('.', '_')
    output.write('<testcase classname="test_%s.%s" name="test_%s" time="1">' % (
        cgi.escape(test_path),
        cgi.escape(test_name.title()),
        cgi.escape(test_name)
    ))
    output.write('<failure type="%s" message="%s"><![CDATA[%s' % (cgi.escape(str(type)), cgi.escape(str(value)), os.linesep))
    traceback.print_exception(type, value, tb, None, output)
    output.write(']]></failure>')
    output.write('</testcase>')

def write_testsuite(output, test_path, filename, exc_info):
    output.write('<testsuite name="pytest" tests="1" errors="0" failures="1" skip="0">')
    write_testcase(output, test_path, filename, exc_info)
    output.write('</testsuite>')

def report_failure(output, test_path, filename, exc_info):
    write_declaration(output)
    write_testsuite(output, test_path, filename, exc_info)

def try_execfile(test_path, filename):
    test_file = os.path.join(test_path, filename)
    if os.path.isfile(test_file):
        report_filename = 'tests.%s.%s.xml' % (test_path.replace('/', '_').replace('\\','_'), filename)
        # clear existing report if there is one
        if os.path.exists(report_filename):
            os.remove(report_filename)
        try:
            exec(compile(open(test_file).read(), test_file, 'exec'), globals())
        except:
            with open(report_filename, 'w') as f:
                report_failure(f, test_path, filename, sys.exc_info())
            raise
