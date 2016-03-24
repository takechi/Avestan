<?xml version="1.0" encoding="Shift_JIS"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output 
	method="html"
	media-type="text/html"
	doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
	encoding="Shift_JIS"/>

<xsl:template match="/">
	<html lang="ja">
		<head>
			<title>Keyboard</title>
		</head>
		<body>
			<xsl:apply-templates/>
		</body>
	</html>
</xsl:template>

<xsl:template match="keyboard">
	<table border="yes">
		<tr><th>修飾子</th><th>キー</th><th>コマンド</th></tr>
		<xsl:apply-templates/>
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
		<xsl:if test="../@name='frame'">
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
