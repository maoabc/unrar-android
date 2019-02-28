package mao.archive.unrar;

public abstract class PasswordCallback implements UnrarCallback {
    @Override
    public final boolean processData(byte[] b, int off, int len) {
        return false;
    }

}
