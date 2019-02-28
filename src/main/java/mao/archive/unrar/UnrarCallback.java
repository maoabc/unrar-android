package mao.archive.unrar;

public interface UnrarCallback {
    boolean processData(byte[] b, int off, int len);

    String needPassword();

    //TODO 分卷解压监听
}
