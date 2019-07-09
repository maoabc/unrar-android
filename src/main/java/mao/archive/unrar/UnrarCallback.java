package mao.archive.unrar;

import androidx.annotation.Keep;

import java.io.Closeable;
import java.io.IOException;

@Keep
public interface UnrarCallback extends Closeable {
    @Keep
    void processData(byte[] b, int off, int len) throws IOException;

    @Keep
    String needPassword();

    //TODO 分卷解压监听
}
