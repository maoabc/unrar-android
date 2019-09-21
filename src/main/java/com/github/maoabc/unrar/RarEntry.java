package com.github.maoabc.unrar;

import androidx.annotation.Keep;

import java.util.Date;

/**
 * Created by mao on 16-10-9.
 */

@Keep
public class RarEntry {
    public static final int RHDF_SPLITBEFORE = 0x01;
    public static final int RHDF_SPLITAFTER = 0x02;
    public static final int RHDF_ENCRYPTED = 0x04;
    public static final int RHDF_SOLID = 0x10;
    public static final int RHDF_DIRECTORY = 0x20;

    private final String name;
    private final long size;  //uncompressed size of entry data unpsize
    private final long csize;    // compressed size of entry data packSize
    private final long crc;
    private final long mtime;
    private final int flags;

    private RarEntry(String name, long size, long csize, long crc, long mtime, int flags) {
        this.name = name;
        this.size = size;
        this.csize = csize;
        this.crc = crc;
        this.mtime = dosToJavaTime(mtime);
        this.flags = flags;
    }

    public String getName() {
        return name;
    }

    public long getSize() {
        return size;
    }

    public long getCsize() {
        return csize;
    }

    public long getCrc() {
        return crc;
    }

    public long getTime() {
        return mtime;
    }

    /*
     * Converts DOS time to Java time (number of milliseconds since epoch).
     */
    private static long dosToJavaTime(long dtime) {
        Date d = new Date((int) (((dtime >> 25) & 0x7f) + 80),
                (int) (((dtime >> 21) & 0x0f) - 1),
                (int) ((dtime >> 16) & 0x1f),
                (int) ((dtime >> 11) & 0x1f),
                (int) ((dtime >> 5) & 0x3f),
                (int) ((dtime << 1) & 0x3e));
        return d.getTime();
    }

    /*
     * Converts Java time to DOS time.
     */
    private static long javaToDosTime(long time) {
        Date d = new Date(time);
        int year = d.getYear() + 1900;
        if (year < 1980) {
            return (1 << 21) | (1 << 16);
        }
        return (year - 1980) << 25 | (d.getMonth() + 1) << 21 |
                d.getDate() << 16 | d.getHours() << 11 | d.getMinutes() << 5 |
                d.getSeconds() >> 1;
    }

    public boolean isDirectory() {
        return isSet(flags, RHDF_DIRECTORY);
    }

    public boolean isEncrypted() {
        return isSet(flags, RHDF_ENCRYPTED);
    }

    public int getFlags() {
        return flags;
    }

    private static boolean isSet(int flags, int b) {
        return (flags & b) == b;
    }
}
