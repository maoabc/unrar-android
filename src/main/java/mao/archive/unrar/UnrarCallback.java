package mao.archive.unrar;

import androidx.annotation.Keep;

@Keep
public interface UnrarCallback {
    @Keep
    boolean processData(byte[] b, int off, int len);

    @Keep
    String needPassword();

    //TODO 分卷解压监听
}
