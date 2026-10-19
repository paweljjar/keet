package com.keet.plugin;

import com.intellij.lang.annotation.AnnotationHolder;
import com.intellij.lang.annotation.ExternalAnnotator;
import com.intellij.lang.annotation.HighlightSeverity;
import com.intellij.openapi.editor.Document;
import com.intellij.openapi.editor.Editor;
import com.intellij.openapi.util.TextRange;
import com.intellij.psi.PsiFile;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class KeetExternalAnnotator extends ExternalAnnotator<PsiFile, List<KeetExternalAnnotator.Diagnostic>> {

    public static class Diagnostic {
        public final int line;
        public final int column;
        public final String message;

        public Diagnostic(int line, int column, String message) {
            this.line = line;
            this.column = column;
            this.message = message;
        }
    }

    @Nullable
    @Override
    public PsiFile collectInformation(@NotNull PsiFile file, @NotNull Editor editor, boolean hasErrors) {
        return file.getFileType() == KeetFileType.INSTANCE ? file : null;
    }

    @Nullable
    @Override
    public List<Diagnostic> doAnnotate(PsiFile file) {
        if (file == null) return null;

        List<Diagnostic> diagnostics = new ArrayList<>();
        File tempFile = null;
        try {
            tempFile = File.createTempFile("keet_lint_", ".keet");
            Files.writeString(tempFile.toPath(), file.getText());

            Process process = new ProcessBuilder("keety", "--lint", "-i", tempFile.getAbsolutePath())
                    .redirectErrorStream(true)
                    .start();

            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            Pattern pattern = Pattern.compile(".*?:(\\d+):(\\d+):\\s*error:\\s*(.*)");

            while ((line = reader.readLine()) != null) {
                Matcher m = pattern.matcher(line);
                if (m.find()) {
                    int lineNum = Integer.parseInt(m.group(1));
                    int colNum = Integer.parseInt(m.group(2));
                    String msg = m.group(3).trim();
                    diagnostics.add(new Diagnostic(lineNum, colNum, msg));
                }
            }
            process.waitFor();
        } catch (Exception ignored) {
        } finally {
            if (tempFile != null && tempFile.exists()) {
                tempFile.delete();
            }
        }
        return diagnostics;
    }

    @Override
    public void apply(@NotNull PsiFile file, @Nullable List<Diagnostic> annotationResult, @NotNull AnnotationHolder holder) {
        if (annotationResult == null || annotationResult.isEmpty()) return;

        Document document = file.getViewProvider().getDocument();
        if (document == null) return;

        for (Diagnostic diag : annotationResult) {
            int lineIndex = Math.max(0, Math.min(diag.line - 1, document.getLineCount() - 1));
            int lineStart = document.getLineStartOffset(lineIndex);
            int lineEnd = document.getLineEndOffset(lineIndex);
            int startOffset = Math.max(lineStart, Math.min(lineStart + diag.column - 1, lineEnd));
            int endOffset = Math.min(startOffset + 1, lineEnd);

            TextRange range = (startOffset < endOffset) ? new TextRange(startOffset, endOffset) : new TextRange(lineStart, lineEnd);

            holder.newAnnotation(HighlightSeverity.ERROR, "KeetScript: " + diag.message)
                    .range(range)
                    .create();
        }
    }
}
