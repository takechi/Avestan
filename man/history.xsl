<?xml version="1.0" encoding="Shift_JIS"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output 
	method="html"
	media-type="text/html"
	doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
	encoding="Shift_JIS"/>

<xsl:template match="log-table">
	<html lang="ja">
		<head>
			<title>更新履歴</title>
			<meta http-equiv="Content-Style-Type" content="text/css"/>
			<link rel="stylesheet" type="text/css" href="manual.css"/>
		</head>
		<body>
			<h1>更新履歴</h1>
			<p>
				<span class="new">新規</span>　<span class="refine">改善</span>　<span class="bugfix">バグ駆除</span>　<span class="internal">内部処理</span>
			</p>
			<dl>
				<xsl:apply-templates/>
			</dl>
		</body>
	</html>
</xsl:template>

<!-- ログ -->
<xsl:template match="log">
	<dt>
		<xsl:value-of select="@version"/>
	</dt>
	<xsl:for-each select="entry">
		<dd>
			<span class="{type}">
				<xsl:choose>
					<xsl:when test="type='new'">★ </xsl:when>
					<xsl:when test="type='refine'">● </xsl:when>
					<xsl:when test="type='bugfix'">◆ </xsl:when>
					<xsl:when test="type='internal'">▲ </xsl:when>
					<xsl:otherwise>
						<xsl:message terminate="yes">
							<xsl:value-of select="type"/> は不正です。
						</xsl:message>
					</xsl:otherwise>
				</xsl:choose>
			</span>
			<xsl:apply-templates select="description"/>
		</dd>
	</xsl:for-each>
</xsl:template>

<xsl:template match="forum">
	<a href="http://lamoo.s53.xrea.com/forum/index.php?all={@id}">[<xsl:value-of select="@id"/>]</a>
</xsl:template>

<xsl:template match="a">
	<a href="{@href}">
		<xsl:choose>
			<xsl:when test="node()">
				<xsl:apply-templates /> 
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="@href" /> 
			</xsl:otherwise>
		</xsl:choose>
	</a>
</xsl:template>

<!-- 基本的にはタグをそのまま表示する -->
<xsl:template match="node()">
	<xsl:copy>
		<xsl:for-each select="@*"><xsl:copy/></xsl:for-each>
		<xsl:apply-templates/>
	</xsl:copy>
</xsl:template>

</xsl:stylesheet>
