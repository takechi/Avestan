Avesta と一緒に利用すると便利な小物類です。
お好みでインストールしてください。

■ OpenWithAvesta.reg
フォルダのコンテキストメニューに「Avestaで開く(&A)」を追加します。
特殊フォルダ（マイコンピュータなど）も開くことができます。
インストール前に、環境に合わせてパスを変更してください。
アンインストールするには、HKEY_CLASSES_ROOT\Folder\shell\OpenWithAvesta 以下を削除します。

■ SetAvestaAsDefault.reg
フォルダの規定の関連付け（ダブルクリックでの動作）を、上の「OpenWithAvesta」に設定します。
アンインストールするには、HKEY_CLASSES_ROOT\Folder\shell の値を空文字にします。

■ OpenLink.reg
ショートカットファイルのリンク先を Avesta で開きます。
インストール前に、環境に合わせてパスを変更してください。
アンインストールするには、HKEY_CLASSES_ROOT\lnkfile\shell\OpenLink 以下を削除します。

■ PromptHere.reg
フォルダのコンテキストメニューに「コマンドプロンプト(&C)」を追加します。
アンインストールするには、HKEY_CLASSES_ROOT\Folder\shell\PromptHere 以下を削除します。
