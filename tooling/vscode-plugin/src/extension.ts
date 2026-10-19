import * as vscode from 'vscode';
import { execFile } from 'child_process';
import * as path from 'path';
import * as fs from 'fs';

let diagnosticCollection: vscode.DiagnosticCollection;

export function activate(context: vscode.ExtensionContext) {
    diagnosticCollection = vscode.languages.createDiagnosticCollection('keetscript');
    context.subscriptions.push(diagnosticCollection);

    // Register active document events for live diagnostics / linting
    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument(document => {
            if (document.languageId === 'keet') {
                lintDocument(document);
            }
        }),
        vscode.workspace.onDidOpenTextDocument(document => {
            if (document.languageId === 'keet') {
                lintDocument(document);
            }
        }),
        vscode.workspace.onDidChangeTextDocument(event => {
            if (event.document.languageId === 'keet') {
                // Clear or re-lint on change with debounce if needed
                lintDocument(event.document);
            }
        }),
        vscode.workspace.onDidCloseTextDocument(document => {
            diagnosticCollection.delete(document.uri);
        })
    );

    // Register AOT Compilation command
    context.subscriptions.push(
        vscode.commands.registerCommand('keetscript.compileComponent', async (uri?: vscode.Uri) => {
            const docUri = uri || vscode.window.activeTextEditor?.document.uri;
            if (!docUri) {
                vscode.window.showErrorMessage('No active KeetScript (.keet) file selected.');
                return;
            }
            await compileKeetFile(docUri, false);
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('keetscript.compileApplication', async (uri?: vscode.Uri) => {
            const docUri = uri || vscode.window.activeTextEditor?.document.uri;
            if (!docUri) {
                vscode.window.showErrorMessage('No active KeetScript (.keet) file selected.');
                return;
            }
            await compileKeetFile(docUri, true);
        })
    );

    // Lint any open keet documents on activation
    vscode.workspace.textDocuments.forEach(doc => {
        if (doc.languageId === 'keet') {
            lintDocument(doc);
        }
    });
}

export function deactivate() {
    if (diagnosticCollection) {
        diagnosticCollection.clear();
        diagnosticCollection.dispose();
    }
}

function resolveKeetyPath(): string | null {
    const config = vscode.workspace.getConfiguration('keetscript');
    const customPath = config.get<string>('compilerPath');
    if (customPath && customPath.trim().length > 0) {
        if (fs.existsSync(customPath)) {
            return customPath;
        }
    }

    // Auto-detect in workspace build folders
    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (workspaceFolders) {
        for (const folder of workspaceFolders) {
            const candidates = [
                path.join(folder.uri.fsPath, 'cmake-build-debug', 'keety'),
                path.join(folder.uri.fsPath, 'build', 'keety'),
                path.join(folder.uri.fsPath, 'build', 'tools', 'keety', 'keety'),
                path.join(folder.uri.fsPath, 'tools', 'keety', 'keety')
            ];
            for (const candidate of candidates) {
                if (fs.existsSync(candidate)) {
                    return candidate;
                }
            }
        }
    }

    return null;
}

function lintDocument(document: vscode.TextDocument) {
    const keetyPath = resolveKeetyPath();
    if (!keetyPath) {
        return; // Silent if compiler is not found yet
    }

    const filePath = document.uri.fsPath;
    if (!fs.existsSync(filePath)) {
        return;
    }

    execFile(keetyPath, ['--lint', '-i', filePath], (error, stdout, stderr) => {
        const diagnostics: vscode.Diagnostic[] = [];
        const output = (stderr || '') + (stdout || '');

        if (error) {
            // Error format: <filename>:<line>:<col>: error: <message>
            // Example: Component.keet:12:5: error: expected identifier
            const regex = /(?:^|\n)(.*?):(\d+):(\d+):\s*(error|warning):\s*(.*)/g;
            let match: RegExpExecArray | null;

            while ((match = regex.exec(output)) !== null) {
                const lineNum = Math.max(0, parseInt(match[2], 10) - 1);
                const colNum = Math.max(0, parseInt(match[3], 10) - 1);
                const isWarning = match[4].toLowerCase() === 'warning';
                const message = match[5].trim();

                const lineText = lineNum < document.lineCount ? document.lineAt(lineNum).text : '';
                const endCol = lineText.length > colNum ? lineText.length : colNum + 1;
                const range = new vscode.Range(lineNum, colNum, lineNum, endCol);

                const diagnostic = new vscode.Diagnostic(
                    range,
                    message,
                    isWarning ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error
                );
                diagnostic.source = 'keety';
                diagnostics.push(diagnostic);
            }

            // Fallback general error if regex did not match
            if (diagnostics.length === 0 && output.trim().length > 0) {
                const range = new vscode.Range(0, 0, 0, 1);
                const diag = new vscode.Diagnostic(range, output.trim(), vscode.DiagnosticSeverity.Error);
                diag.source = 'keety';
                diagnostics.push(diag);
            }
        }

        diagnosticCollection.set(document.uri, diagnostics);
    });
}

async function compileKeetFile(uri: vscode.Uri, appMode: boolean) {
    const keetyPath = resolveKeetyPath();
    if (!keetyPath) {
        vscode.window.showErrorMessage(
            'keety compiler executable not found. Please build keet or set "keetscript.compilerPath" in settings.'
        );
        return;
    }

    const inputPath = uri.fsPath;
    const parsed = path.parse(inputPath);
    const outDir = path.join(parsed.dir, 'generated');
    if (!fs.existsSync(outDir)) {
        fs.mkdirSync(outDir, { recursive: true });
    }

    const outSource = path.join(outDir, `${parsed.name}.cpp`);
    const outHeader = path.join(outDir, `${parsed.name}.h`);

    const args = ['-i', inputPath, '-o', outSource, '-h', outHeader, '-c', parsed.name];
    if (appMode) {
        args.unshift('--app');
    }

    vscode.window.withProgress(
        {
            location: vscode.ProgressLocation.Notification,
            title: `Compiling ${parsed.base} via keety ${appMode ? '(--app mode)' : ''}...`,
            cancellable: false
        },
        () => {
            return new Promise<void>((resolve, reject) => {
                execFile(keetyPath, args, (error, stdout, stderr) => {
                    if (error) {
                        const errMsg = (stderr || stdout || error.message).trim();
                        vscode.window.showErrorMessage(`Compilation failed: ${errMsg}`);
                        reject(error);
                    } else {
                        vscode.window.showInformationMessage(
                            `Successfully compiled ${parsed.base} -> generated/${parsed.name}.cpp & generated/${parsed.name}.h 🐱`
                        );
                        resolve();
                    }
                });
            });
        }
    );
}
