package mao.archive.unrar;


/**
 * 批量解压过滤
 * Created by mao on 17-6-6.
 */
public interface ProgressListener {
    void onProgressing(String fileName, long size);
}
