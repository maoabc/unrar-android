package com.github.maoabc.unrar;

import android.content.Context;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import static org.junit.Assert.assertEquals;

/**
 * Instrumentation test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {

    private File cacheDir;

    @Before
    public void useAppContext() throws Exception {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getTargetContext();

        assertEquals("mao.archive.unrar.test", appContext.getPackageName());
        cacheDir = appContext.getCacheDir();
    }

    private File getTestRarFile(String name) throws IOException {
        File file = new File(cacheDir, name);
        if (file.exists()) {
            return file;
        }
        try (
                InputStream inputStream = getClass().getResourceAsStream("/" + name);
                FileOutputStream outputStream = new FileOutputStream(file);
        ) {
            copyStream(inputStream, outputStream);
        }
        return file;
    }

    @Test
    public void testListEntries() throws IOException {
        File testRarFile = getTestRarFile("testRar5.rar");
        RarFile rarFile = new RarFile(testRarFile);

        for (RarEntry entry : rarFile.getEntries(null)) {
            String name = entry.getName();
            System.out.println(name);
        }

    }

    @Test
    public void testExtract() throws IOException {
        File testRarFile = getTestRarFile("testRar5.rar");
        RarFile rarFile = new RarFile(testRarFile);
        File out = new File(cacheDir, "out");
        if (!out.exists()) {
            out.mkdirs();
        }
        rarFile.extractAll(out.getAbsolutePath(), null);

    }


    @Test
    public void testExtractInMem() throws IOException {
        File testRarFile = getTestRarFile("name_encrypted.rar");
        RarFile rarFile = new RarFile(testRarFile);
        rarFile.extract("网游昵称大全.txt", new UnrarCallback() {
            @Override
            public void close() throws IOException {

            }

            @Override
            public void processData(byte[] b, int off, int len) throws IOException {
                System.out.println(new String(b, off, len, "GBK"));
            }

            @Override
            public String needPassword() {
                return "190512";
            }
        });

    }

    public static void copyStream(InputStream is, OutputStream os)
            throws IOException {
        byte[] buff = new byte[4096];
        int rc;
        while ((rc = is.read(buff)) != -1) {
            os.write(buff, 0, rc);
        }
        os.flush();
    }

}
