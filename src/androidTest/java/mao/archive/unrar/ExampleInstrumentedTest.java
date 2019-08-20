package mao.archive.unrar;

import android.content.Context;
import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

import static org.junit.Assert.*;

/**
 * Instrumentation test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    @Test
    public void useAppContext() throws Exception {
        // Context of the app under test.
        Context appContext = InstrumentationRegistry.getTargetContext();

        assertEquals("mao.archive.unrar.test", appContext.getPackageName());
    }

    @Test
    public void testEncrypt() throws IOException {
        RarFile rarFile = new RarFile("/sdcard/All.rar");

        for (RarEntry entry : rarFile.getEntries(null)) {
            String name = entry.getName();
            System.out.println(name);
        }

    }

    @Test
    public void testExtract() throws IOException {
        RarFile rarFile = new RarFile("/sdcard/All.rar");
        rarFile.extractAll("/sdcard/zhihu", null);

    }


    @Test
    public void testExtractInMem() throws IOException {
        RarFile rarFile = new RarFile("/sdcard/name_encrypted.rar");
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

}
