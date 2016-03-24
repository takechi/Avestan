<?xml version="1.0" encoding="Shift_JIS"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output 
	method="html"
	media-type="text/html"
	doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
	encoding="Shift_JIS"/>

<xsl:template match="/structure">
	<html lang="ja">
		<head>
			<title>キー操作一覧</title>
			<meta http-equiv="Content-Style-Type" content="text/css"/>
			<link rel="stylesheet" type="text/css" href="manual.css"/>
		</head>
		<body>
			<h1>キー操作一覧</h1>
			<p>
				AVESTA/usr/keyboard-*.xml を編集することで変更できます。
				キー名は大文字・小文字を区別しません。
			</p>
			<xsl:apply-templates/>
		</body>
	</html>
</xsl:template>

<xsl:template match="structure/case">
	<h2><xsl:value-of select="@name"/></h2>
	<table>
		<tr>
			<th>修飾子</th>
			<th>キー</th>
			<xsl:for-each select="widget">
				<th>
					<xsl:value-of select="@name"/><br />
					<kbd><a href="{@file}">(<xsl:value-of select="substring-after(@file, 'usr/')"/>)</a></kbd>
				</th>
			</xsl:for-each>
		</tr>
		<xsl:apply-templates select="document(widget[1]/@file)/keyboard/bind | document(widget[2]/@file)/keyboard/bind">
			<xsl:sort select="string-length(@key)"/>
			<xsl:sort select="@key"/>
			<xsl:sort select="@modifier"/>
		</xsl:apply-templates>
	</table>
</xsl:template>


<xsl:template match="//keyboard/bind">
	<tr>
		<td>
			<xsl:if test="contains(@modifier, 'C') = true()">Ctrl</xsl:if>
			<xsl:if test="contains(@modifier, 'S') = true()">
				<xsl:if test="contains(@modifier, 'C') = true()"> + </xsl:if>
				Shift
			</xsl:if>
			<xsl:if test="contains(@modifier, 'A') = true()">
				<xsl:if test="contains(@modifier, 'C') = true() or contains(@modifier, 'S') = true()"> + </xsl:if>
				Alt
			</xsl:if>
			　
		</td>
		<td align="center"><xsl:value-of select="@key"/></td>
		<xsl:if test="../@name!='form'">
			<td>　</td>
		</xsl:if>
		<td class="command">
			<xsl:choose>
				<xsl:when test="boolean(@command)">
					<xsl:value-of select="@command"/>
				</xsl:when>
				<xsl:when test="boolean(@execute)">
					<xsl:value-of select="@execute"/> ( <xsl:value-of select="@args"/> )
				</xsl:when>
				<xsl:when test="boolean(@navigate)">
					<xsl:value-of select="@navigate"/> ( <xsl:value-of select="@args"/> )
				</xsl:when>
				<xsl:otherwise>
					<dl>
						<xsl:apply-templates/>
					</dl>
				</xsl:otherwise>
			</xsl:choose>
		</td>
		<xsl:if test="../@name='form'">
			<td>　</td>
		</xsl:if>
	</tr>
</xsl:template>

<xsl:template match="menu">
	<dt>
		<xsl:value-of select="@text"/>
	</dt>
	<dd>
		<xsl:choose>
			<xsl:when test="boolean(@command)">
				<xsl:value-of select="@command"/>
			</xsl:when>
			<xsl:when test="boolean(@execute)">
				<xsl:value-of select="@execute"/> ( <xsl:value-of select="@args"/> )
			</xsl:when>
			<xsl:when test="boolean(@navigate)">
				<xsl:value-of select="@navigate"/> ( <xsl:value-of select="@args"/> )
			</xsl:when>
		</xsl:choose>
	</dd>
</xsl:template>

</xsl:stylesheet>
