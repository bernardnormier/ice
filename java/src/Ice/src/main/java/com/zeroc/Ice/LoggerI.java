//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

/**
 * @hidden Public because it's used by IceGridGUI.
 */
public class LoggerI implements Logger {
    public LoggerI(String prefix, String file) {
        _prefix = prefix;

        if (!prefix.isEmpty()) {
            _formattedPrefix = prefix + ": ";
        }

        _lineSeparator = System.getProperty("line.separator");
        _date = java.text.DateFormat.getDateInstance(java.text.DateFormat.SHORT);
        _time = new java.text.SimpleDateFormat(" HH:mm:ss:SSS");

        if (!file.isEmpty()) {
            _file = file;
            try {
                _out = new java.io.FileOutputStream(new java.io.File(_file), true);
            } catch (java.io.FileNotFoundException ex) {
                throw new InitializationException("FileLogger: cannot open " + _file);
            }
        }
    }

    @Override
    public void print(String message) {
        StringBuilder s = new StringBuilder(256);
        s.append(message);
        write(s, false);
    }

    @Override
    public void trace(String category, String message) {
        StringBuilder s = new StringBuilder(256);
        s.append("-- ");
        synchronized (this) {
            java.util.Date date = new java.util.Date();
            s.append(_date.format(date));
            s.append(_time.format(date));
        }
        s.append(' ');
        s.append(_formattedPrefix);
        s.append(category);
        s.append(": ");
        s.append(message);
        write(s, true);
    }

    @Override
    public void warning(String message) {
        StringBuilder s = new StringBuilder(256);
        s.append("-! ");
        synchronized (this) {
            s.append(_date.format(new java.util.Date()));
            s.append(_time.format(new java.util.Date()));
        }
        s.append(' ');
        s.append(_formattedPrefix);
        s.append("warning: ");
        s.append(Thread.currentThread().getName());
        s.append(": ");
        s.append(message);
        write(s, true);
    }

    @Override
    public void error(String message) {
        StringBuilder s = new StringBuilder(256);
        s.append("!! ");
        synchronized (this) {
            s.append(_date.format(new java.util.Date()));
            s.append(_time.format(new java.util.Date()));
        }
        s.append(' ');
        s.append(_formattedPrefix);
        s.append("error: ");
        s.append(Thread.currentThread().getName());
        s.append(": ");
        s.append(message);
        write(s, true);
    }

    @Override
    public String getPrefix() {
        return _prefix;
    }

    @Override
    public Logger cloneWithPrefix(String prefix) {
        return new LoggerI(prefix, _file);
    }

    private void write(StringBuilder message, boolean indent) {
        if (indent) {
            int idx = 0;
            while ((idx = message.indexOf("\n", idx)) != -1) {
                message.insert(idx + 1, "   ");
                ++idx;
            }
        }
        message.append(_lineSeparator);

        if (_out == null) {
            System.err.print(message.toString());
        } else {
            try {
                _out.write(message.toString().getBytes());
            } catch (java.io.IOException ex) {
            }
        }
    }

    public void destroy() {
        if (!_file.isEmpty()) {
            try {
                _out.close();
            } catch (java.io.IOException ex) {
            }
        }
    }

    String _prefix = "";
    String _formattedPrefix = "";
    String _file = "";
    String _lineSeparator;
    java.text.DateFormat _date;
    java.text.SimpleDateFormat _time;
    java.io.FileOutputStream _out = null;
}
