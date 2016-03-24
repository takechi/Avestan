このフォルダには、外部拡張ツールを置くことを想定しています。
もちろん、このフォルダ以外の場所にあるツールを使うこともできます。

付属のスクリプトは、それほど実用性は高くないと思います。
お好みのツールを追加してください。

■■■ dumptree.bat ■■■
【用意するもの】
    とくになし
【登録の仕方】
    <XXX execute="AVESTA\opt\dumptree.cmd" />
【説明】
    カレントディレクトリ以下のファイルも含めたツリーを dumptree.txt へ出力する。

■■■ ExtractURL.pyw ■■■
【用意するもの】
    Python
【登録の仕方】
    <XXX execute="AVESTA/opt/ExtractURL.pyw" args="{selected}" />
【説明】
    選択されたテキストファイルからURLを抽出し、クリップボードへコピーします。

■■■ jpeg2jpg.bat ■■■
【用意するもの】
    とくになし
【登録の仕方】
    <XXX execute="AVESTA\opt\jpeg2jpg.cmd" />
【説明】
    自分の環境だと、JPEGファイルを保存すると、なぜか勝手に拡張子が .jpeg になるので……。

■■■ rmemptydir.cmd ■■■
【用意するもの】
    とくになし
【登録の仕方】
    <XXX execute="AVESTA\opt\rmemptydir.cmd" />
【説明】
    カレントディレクトリ以下の空のディレクトリを再帰的に削除する。
    「空のディレクトリのみを含むディレクトリ」も削除することに注意。
    （本当は、「注意」というよりもこの動作になるように小細工をしているのだが）

■■■ rmemptydir.py ■■■
【用意するもの】
    Python
【登録の仕方】
    <XXX execute="AVESTA\opt\rmemptydir.py" />
【説明】
    rmemptydir.cmd の Python 版。
    一時ファイルを作成しないこと以外はほぼ同等。

■■■ SummonBash.bat / SummonBash.bash_login.txt ■■■
【用意するもの】
    cygwin (http://www.cygwin.com/)
【登録の仕方】
    ・登録の仕方：<XXX execute="AVESTA\opt\SummonBash.bat" args="{current}" />
【説明】
    cygwin bash を呼ぶ。
    SummonBash.bash_login.txt の内容を、.bash_login に追加する必要がある。
    （.bashrc に変更している人はそちらに）
    cygwin が C:\cygwin\bin 以外にインストールしてある場合は修正も必要。
