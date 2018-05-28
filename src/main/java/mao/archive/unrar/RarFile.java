package mao.archive.unrar;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Iterator;

/**
 * Created by mao on 16-10-9.
 */

public class RarFile {

    //
    private static final int RAR_OM_LIST = 0;
    private static final int RAR_OM_EXTRACT = 1;
    private static final int RAR_OM_LIST_INCSPLIT = 2;


    private static final String TAG = "RarFile";

    static {
        System.loadLibrary("unrar-jni");
        initIDs();
    }

    //只能判断rar文件名是否加密,如果只加密数据还要根据头进一部判断
    private final boolean encrypted;

    private final String rarName;  //Rar文件路径
    private String password;


    public RarFile(String rarName, String password) {
        this.rarName = rarName;
        this.password = password;
        encrypted = checkEncrypted();
    }

    public RarFile(String rarName) {
        this(rarName, null);
    }

    public RarFile(File file) throws IOException {
        this(file.getAbsolutePath(), null);
    }

    public RarFile(File file, String password) throws IOException {
        this(file.getAbsolutePath(), password);
    }

    /**
     * 列出rar内所有文件
     *
     * @return
     * @throws IOException
     */
    public Iterable<RarEntry> entries() throws IOException {
        final long handle = openArchive(rarName, RAR_OM_LIST, null);

        if (encrypted) {
            setPassword(handle, password);
        }
        return new Iterable<RarEntry>() {
            public Iterator<RarEntry> iterator() {
                return new Iterator<RarEntry>() {
                    volatile boolean closed;
                    RarEntry entry;

                    public boolean hasNext() {
                        entry = readHeaderSkipData(handle);
                        if (entry == null) {
                            close();
                            return false;
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
                        closeArchive(handle);
                    }

                    @Override
                    protected void finalize() throws Throwable {
                        //gc 清除当前对象时,如果还没关闭handle,关闭它
                        close();
                    }
                };
            }
        };
    }

    /**
     * 解压单个文件,返回字节数组
     *
     * @param entryName rar内文件名
     * @return 解压后的数据
     * @throws IOException
     */
    public byte[] extractOne(String entryName) throws IOException {
        final long handle = openArchive(rarName, RAR_OM_EXTRACT, null);
        try {
            if (encrypted) {
                setPassword(handle, password);
            }
            RarEntry header;
            while ((header = readHeader(handle)) != null) {
                if (header.getName().equals(entryName)) {
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream((int) header.getSize());
                    readData(handle, null, outputStream);
                    return outputStream.toByteArray();
                } else {
                    readData(handle, null, null);
                }
            }
        } finally {
            closeArchive(handle);
        }
        return null;
    }

    /**
     * 解压单个文件,数据写入output stream
     *
     * @param entryName    rar内文件名
     * @param outputStream 接收解压出的数据
     * @return 返回结果
     * @throws IOException
     */

    public void extractOne(String entryName, OutputStream outputStream) throws IOException {
        if (outputStream == null) {
            throw new NullPointerException("output stream is null");
        }
        final long handle = openArchive(rarName, RAR_OM_EXTRACT, null);

        try {

            if (encrypted) {
                setPassword(handle, password);
            }
            RarEntry header;
            while ((header = readHeader(handle)) != null) {
                if (header.getName().equals(entryName)) {
                    readData(handle, null, outputStream);
                    return;
                }
                readData(handle, null, null);
            }
        } finally {
            closeArchive(handle);
        }
    }


    /**
     * 解压所有数据到某个目录
     *
     * @param destPath         解压目标目录
     * @param progressListener 解压进度监听
     * @throws IOException
     */
    public void extractAllToDestPath(String destPath, ProgressListener progressListener) throws IOException {
        extractBatchDestPath(destPath, new ExtractFilter() {
            @Override
            public boolean filter(RarEntry entry) {
                return true;
            }
        }, progressListener);
    }


    /**
     * 批量解压数据到某个目录
     *
     * @param destPath         解压目标目录
     * @param filter           对解压数据进行过滤
     * @param progressListener 解压进度监听
     * @throws IOException
     */
    public void extractBatchDestPath(String destPath, ExtractFilter filter, ProgressListener progressListener) throws IOException {
        final long handle = openArchive(rarName, RAR_OM_EXTRACT, null);
        try {
            if (encrypted) {
                setPassword(handle, password);
            }
            RarEntry header;
            while ((header = readHeader(handle)) != null) {
                if (filter == null || filter.filter(header)) {
                    if (progressListener != null)
                        progressListener.onProgressing(header.getName(), header.getSize());
                    readData(handle, destPath, null);
                } else {
                    //跳过解压数据
                    readData(handle, null, null);
                }
            }
        } finally {
            closeArchive(handle);
        }
    }

    //检查rar文件名是否被加密
    private synchronized boolean checkEncrypted() {
        long handle = -1;
        try {
            boolean[] encrypted = new boolean[1];
            handle = openArchive(rarName, RAR_OM_LIST, encrypted);
            if (encrypted[0]) {
                return true;
            }
            RarEntry header;
            while ((header = readHeader(handle)) != null) {
                if (header.isEncrypted()) {
                    return true;
                }
                readData(handle, null, null);
            }
        } catch (IOException ignored) {
        } finally {
            if (handle != -1) closeArchive(handle);
        }
        return false;
    }

    public boolean isEncrypted() {
        return encrypted;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    private static native void initIDs();


    private static native long openArchive(String rarName, int mode, boolean[] encrypted) throws RarException;

    //读取rar头
    private static native RarEntry readHeader(long handle);


    //读取rar头,忽略实际数据
    private static native RarEntry readHeaderSkipData(long handle);


    private static native void setPassword(long handle, String passwd);


    private static native int readData(long handle, String destPath, OutputStream output) throws IOException;


    private static native void closeArchive(long handle);


}
