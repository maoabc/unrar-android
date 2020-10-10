package com.github.maoabc.unrar;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Iterator;

/**
 * Created by mao on 16-10-9.
 */

public class RarFile {

    //
    private static final int RAR_OM_LIST = 0;
    private static final int RAR_OM_EXTRACT = 1;
    private static final int RAR_OM_LIST_INCSPLIT = 2;


    private static final int RAR_SKIP = 0;
    private static final int RAR_TEST = 1;
    private static final int RAR_EXTRACT = 2;


    static {
        System.loadLibrary("unrar-jni");
    }

    //只能判断rar文件名是否加密,如果只加密数据还要根据头进一部判断
//    private final boolean encrypted;

    private final String rarPath;  //Rar文件路径


    public RarFile(String rarPath) {
        this.rarPath = rarPath;
    }

    public RarFile(File file) throws IOException {
        this(file.getCanonicalPath());
    }

    /**
     * 列出rar内所有文件
     *
     * @return rarEntry迭代器
     * @throws IOException 打开rar异常
     */
    public Iterable<RarEntry> getEntries(final UnrarCallback callback) throws IOException {
        final long handle = openArchive(rarPath, RAR_OM_LIST);

        return new Iterable<RarEntry>() {
            @NonNull
            public Iterator<RarEntry> iterator() {
                return new Iterator<RarEntry>() {
                    volatile boolean closed;
                    RarEntry entry;

                    public boolean hasNext() {
                        entry = readHeader0(handle, callback);
                        if (entry == null) {
                            close();
                            return false;
                        }
                        try {
                            processFile0(handle, RAR_SKIP, null, null, null);
                        } catch (IOException ignored) {
                            //todo except
                        }
                        return true;
                    }

                    public RarEntry next() {
                        return entry;
                    }

                    public void remove() {

                    }

                    private void close() {
                        if (closed) {
                            return;
                        }
                        closed = true;
                        try {
                            closeArchive(handle);
                        } catch (IOException ignored) {
                        }
                    }

                    @Override
                    protected void finalize() throws Throwable {
                        close();
                    }
                };
            }
        };
    }


    /**
     * @param entryName 文件名
     * @param callback  密码或者解压回调
     * @throws IOException rar文件异常
     */
    public void extract(String entryName, UnrarCallback callback) throws IOException {

        final long handle = openArchive(rarPath, RAR_OM_EXTRACT);

        try {

            RarEntry header;
            while ((header = readHeader0(handle, callback)) != null) {
                if (header.getName().equals(entryName)) {
                    processFile0(handle, RAR_TEST, null, null, callback);
                    break;
                } else {//skip
                    processFile0(handle, RAR_SKIP, null, null, null);
                }
            }
        } finally {
            closeArchive(handle);
        }
    }

    /**
     * 返回字节流，少用，内存占用太高
     *
     * @param entry    文件名
     * @param password 解压密码，无密码为null
     * @return 文件流
     * @throws IOException 打开rar异常
     */
    public InputStream getInputStream(String entry, final String password) throws IOException {
        final ByteArrayOutputStream out = new ByteArrayOutputStream();
        extract(entry, new UnrarCallback() {
            @Override
            public void processData(ByteBuffer buffer, int len) throws IOException {
                byte[] bytes = new byte[len];
                buffer.get(bytes);
                out.write(bytes, 0, len);
            }

            @Override
            public String needPassword() {
                return password;
            }

            @Override
            public void close() throws IOException {
            }
        });
        return new ByteArrayInputStream(out.toByteArray());
    }


    /**
     * 解压所有数据到某个目录
     *
     * @param destPath 解压目标目录
     * @throws IOException
     */
    public void extractAll(String destPath, UnrarCallback callback) throws IOException {
        extractBatch(destPath, callback, new ExtractFilter() {
            @Override
            public boolean accept(RarEntry entry) {
                return true;
            }
        });
    }


    /**
     * 批量解压数据到某个目录
     *
     * @param destPath 解压目标目录
     * @param filter   对解压文件进行过滤
     * @throws IOException
     */
    public void extractBatch(String destPath, UnrarCallback callback, ExtractFilter filter) throws IOException {
        final long handle = openArchive(rarPath, RAR_OM_EXTRACT);
        try {
            RarEntry header;
            while ((header = readHeader0(handle, callback)) != null) {
                if (filter != null && filter.accept(header)) {
                    processFile0(handle, RAR_EXTRACT, destPath, null, null);
                } else {
                    processFile0(handle, RAR_SKIP, null, null, null);
                }
            }
        } finally {
            closeArchive(handle);
        }
    }

    /**
     * 批量解压数据到某个目录
     *
     * @param filter 对解压文件进行过滤
     * @throws IOException
     */
    public void extractBatch2(PasswordCallback passwordCallback, ExtractFilter filter, OnCreateOutCallback outCallback) throws IOException {
        if (outCallback == null) {
            throw new IOException();
        }
        final long handle = openArchive(rarPath, RAR_OM_EXTRACT);
        try {
            RarEntry header;
            while ((header = readHeader0(handle, passwordCallback)) != null) {
                if (filter != null && filter.accept(header)) {
                    if (!header.isDirectory()) {
                        try (UnrarCallback out = outCallback.createOut(header.getName());
                        ) {
                            processFile0(handle, RAR_TEST, null, null, out);
                        }
                    }
                } else {
                    processFile0(handle, RAR_SKIP, null, null, null);
                }
            }
        } finally {
            closeArchive(handle);
        }
    }


    @Keep
    private static native long openArchive(String rarName, int mode) throws RarException;

    //读取rar头
    @Keep
    private static native RarEntry readHeader0(long handle, UnrarCallback callback);


    @Keep
    private static native void processFile0(long handle, int operation, String destPath, String destName, UnrarCallback callback) throws IOException;


    @Keep
    private static native void closeArchive(long handle) throws IOException;


}
