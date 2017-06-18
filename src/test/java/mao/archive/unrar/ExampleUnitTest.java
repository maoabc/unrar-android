package mao.archive.unrar;

import org.junit.Test;

import java.io.IOException;
import java.io.OutputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;

import static org.junit.Assert.assertEquals;

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class ExampleUnitTest {
    @Test
    public void addition_isCorrect() throws Exception {
        assertEquals(4, 2 + 2);
    }


    @Test
    public void testIterEntries() throws IOException {
        RarFile rarFile = new RarFile("/home/mao/Downloads/MetalSlugAll.rar");
        if (rarFile.isEncrypted()) {
            rarFile.setPassword("190512");
        }
        for (RarEntry entry : rarFile.entries()) {
            System.out.println(entry.getName());
        }

    }

    @Test
    public void testExtractOne() throws IOException {
        RarFile rarFile = new RarFile("/home/mao/Downloads/MetalSlugAll.rar");
        if (rarFile.isEncrypted()) {
            rarFile.setPassword("190512");
        }
        byte[] bytes = rarFile.extractOne("MetalSlugAll/ini/mslug4.ini");
        System.out.println(new String(bytes, "GBK"));
    }

    //测试异常是否正确被捕获
    @Test
    public void testExtractOne2() throws IOException {
        RarFile rarFile = new RarFile("/home/mao/Downloads/MetalSlugAll.rar");
        if (rarFile.isEncrypted()) {
            rarFile.setPassword("190512");
        }
        rarFile.extractOne("MetalSlugAll/ini/mslug4.ini", new OutputStream() {
            @Override
            public void write(int b) throws IOException {

            }
            @Override
            public void write(byte[] b, int off, int len) throws IOException {
                throw new IOException("excpt ");
            }
        });

    }


    //测试通过管道读取数据
    @Test
    public void testExtractOneByPipeStream() throws IOException, InterruptedException {
        final RarFile rarFile = new RarFile("/home/mao/Downloads/MetalSlugAll.rar");
        if (rarFile.isEncrypted()) {
            rarFile.setPassword("190512");
        }
        PipedInputStream pipedInputStream = new PipedInputStream(60*1024);
        final PipedOutputStream pipedOutputStream = new PipedOutputStream(pipedInputStream);
        Thread thread = new Thread(new Runnable() {
            public void run() {
                try {
                    rarFile.extractOne("MetalSlugAll/roms/mslug4.zip", pipedOutputStream);
                    pipedOutputStream.close();
                } catch (IOException e) {
                }
            }
        });
        thread.start();

        byte[] bytes = new byte[1024*64];
        int len;
        while ((len=pipedInputStream.read(bytes))!=-1){
            System.out.println("read "+len);
        }


    }

    @Test
    public void testExtractAllToDestPath() throws IOException {
        RarFile rarFile = new RarFile("/home/mao/Downloads/MetalSlugAll.rar");
        if (rarFile.isEncrypted()) {
            rarFile.setPassword("190512");
        }
        rarFile.extractAllToDestPath("/home/mao/out", new ProgressListener() {
            public void onProgressing(String fileName, long size) {
                System.out.println(fileName + "   " + size);
            }
        });
    }
}