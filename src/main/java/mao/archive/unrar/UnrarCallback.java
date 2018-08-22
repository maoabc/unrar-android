package mao.archive.unrar;

import java.io.IOException;

public interface UnrarCallback {
    boolean processData(byte[] b, int off, int len) throws IOException;

    String needPassword();

    //TODO 分卷解压监听
}
