<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<!-- 
   $Log: trick_struct_tex.xsl,v $
   Revision 8.1  2009-08-20 14:19:57-05  dmidura
   Bump version number for new group TrickHLA_2.2

   Revision 7.1  2009-08-05 10:32:20-05  dmidura
   Bump version number for new group TrickHLA_2.1

   Revision 6.1  2009-06-29 13:28:34-05  dmidura
   Bump version number for new group TrickHLA_2.0

   Revision 5.2  2009-06-25 17:49:22-05  ddexter
   Update documentation.

   Revision 3.1  2008-08-25 12:11:20-05  razor
   Bump version number for new group TrickHLA_1.4

   Revision 2.1  2008-08-14 11:21:14-05  dmidura
   Bump version number for new group TrickHLA_1.3

   Revision 1.3  2008-04-04 19:03:44-05  ddexter
   Files from v1.1

   Revision 1.2  2008-04-04 18:43:11-05  ddexter
   bumping the version

   Revision 1.1  2007-05-23 15:45:04-05  ddexter
   Initial revision

   Revision 1.2  2006-03-14 09:12:20-06  slim
   Backslash Fix

   Revision 1.1  2005-10-26 16:55:08-05  ezcrues
   Initial revision

-->

<xsl:template match="/file">
    <xsl:apply-templates select="file_name"/>
    <xsl:apply-templates select="trick_header"/>

    <xsl:if test="count(enumerations/enum) != 0"> 
{\bf Enumerations:}
\begin{itemize} <xsl:for-each select="enumerations/enum">
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="enum_name"/></xsl:call-template>
        </xsl:for-each>
\end{itemize}
    </xsl:if>

    <xsl:if test="count(structures/struct) != 0"> 
{\bf Structures:}
\begin{itemize} <xsl:for-each select="structures/struct">
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="struct_name"/></xsl:call-template>
        </xsl:for-each>
\end{itemize}
    </xsl:if>
    <xsl:apply-templates select="enumerations"/>
    <xsl:apply-templates select="structures"/>
</xsl:template>

<xsl:template match="file_name">
{\bf File:} {\tt <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="text()"/></xsl:call-template> }
\vspace{0.125in}
</xsl:template>

<!-- write out the trick header -->
<xsl:template match="trick_header">
    <xsl:apply-templates select="purpose"/>
    <xsl:apply-templates select="references"/>
    <xsl:apply-templates select="assumptions"/>
</xsl:template>

<xsl:template match="purpose">
{\bf Purpose:}
    <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="text()"/></xsl:call-template>
\vspace{0.125in}
</xsl:template>

<xsl:template match="references">
{\bf References:}
    <xsl:if test="count(reference) != 0">
\begin{itemize} <xsl:apply-templates select="reference"/>
\end{itemize}
</xsl:if>
</xsl:template>

<xsl:template match="reference">
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="author"/></xsl:call-template>
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="source"/></xsl:call-template>
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="id"/></xsl:call-template>
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="location"/></xsl:call-template>
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="date"/></xsl:call-template>
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="notes"/></xsl:call-template>
</xsl:template>

<xsl:template match="assumptions">
{\bf Assumption and Limitations:}
    <xsl:if test="count(assumption) != 0">
\begin{itemize} <xsl:apply-templates select="assumption"/>
\end{itemize}
    </xsl:if>
</xsl:template>

<xsl:template match="assumption">
\item <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="text()"/></xsl:call-template>
</xsl:template>


<xsl:template match="enumerations">
    <xsl:apply-templates select="enum"/>
</xsl:template>

<xsl:template match="enum">
\begin{table}[hbt]
\begin{center}
\begin{tabular}{|l|l|p{3.0in}|} \hline
\multicolumn{3}{|c|}{\bf <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="enum_name"/></xsl:call-template>} \\ \hline \hline
{\em Tag Name} &amp; {\em Value} &amp; {\em Description} \\ \hline
<!--    <xsl:apply-templates select="file/enumerations/enum/member"/> -->
   <xsl:for-each select="member">
      <!--  <td><xsl:value-of select="name"/></td>
            <td><xsl:value-of select="value"/></td>
            <td><xsl:value-of select="comment"/></td> -->
      {\tt <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="name"/></xsl:call-template> } &amp;
      <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="value"/></xsl:call-template> &amp;
      <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="comment"/></xsl:call-template> \\ \hline
   </xsl:for-each>
\end{tabular}
\end{center}
\caption{<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="enum_name"/></xsl:call-template> enumeration tag values.}
\label{tbl:<xsl:value-of select="enum_name"/>}
\end{table}
</xsl:template>

<xsl:template match="file/enumerations/enum/member">
{\tt <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="name"/></xsl:call-template> } &amp;
<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="value"/></xsl:call-template> &amp;
<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="comment"/></xsl:call-template> \\ \hline
</xsl:template>

<xsl:template match="structures">
    <xsl:apply-templates select="struct"/>
</xsl:template>

<xsl:template match="struct">
\begin{table}[hbt]
\begin{center}
\begin{tabular}{|l|l|l|l|p{2.75in}|} \hline
\multicolumn{5}{|c|}{\bf <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="struct_name"/></xsl:call-template>} \\ \hline \hline
{\em Field Name} &amp; {\em Type} &amp; {\em Dim} &amp; {\em Units} &amp; {\em Description} \\ \hline
    <xsl:apply-templates select="member"/>
\end{tabular}
\end{center}
\caption{<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="struct_name"/></xsl:call-template> field members.}
\label{tbl:<xsl:value-of select="struct_name"/>}
\end{table}
</xsl:template>

<xsl:template match="member">
{\tt <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="name"/></xsl:call-template> } &amp;
{\tt <xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="type"/></xsl:call-template> } &amp;
<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="dims"/></xsl:call-template> &amp;
<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="units"/></xsl:call-template> &amp;
<xsl:call-template name="transform_special"><xsl:with-param name="incoming" select="comment"/></xsl:call-template> \\ \hline
</xsl:template>

<xsl:template name="replace">
  <xsl:param name="string" select="''"/>
  <xsl:param name="pattern" select="''"/>
  <xsl:param name="replacement" select="''"/>
  <xsl:choose>
    <xsl:when test="$pattern != '' and $string != '' and contains($string, $pattern)">
      <xsl:value-of select="substring-before($string, $pattern)"/>
      <xsl:copy-of select="$replacement"/>
      <xsl:call-template name="replace">
        <xsl:with-param name="string" select="substring-after($string, $pattern)"/>
        <xsl:with-param name="pattern" select="$pattern"/>
        <xsl:with-param name="replacement" select="$replacement"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$string"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="transform_special">
  <xsl:param name="incoming" select="''"/>
  <xsl:call-template name="replace">
    <xsl:with-param name="string">
      <xsl:call-template name="replace">
        <xsl:with-param name="string">
          <xsl:call-template name="replace">
            <xsl:with-param name="string">
              <xsl:call-template name="replace">
                <xsl:with-param name="string">
                  <xsl:call-template name="replace">
                    <xsl:with-param name="string">
                      <xsl:call-template name="replace">
                        <xsl:with-param name="string">
                          <xsl:call-template name="replace">
                            <xsl:with-param name="string">
                              <xsl:call-template name="replace">
                                <xsl:with-param name="string">
                                  <xsl:call-template name="replace">
                                    <xsl:with-param name="string">
                                      <xsl:call-template name="replace">
                                        <xsl:with-param name="string">
                                          <xsl:call-template name="replace">
                                            <xsl:with-param name="string">
                                              <xsl:call-template name="replace">
                                                <xsl:with-param name="string" select="normalize-space($incoming)"/>
                                                <xsl:with-param name="pattern" select="'\'"/>
                                                <xsl:with-param name="replacement" select="'$\backslash$'"/>
                                              </xsl:call-template>
                                            </xsl:with-param>
                                            <xsl:with-param name="pattern" select="'&amp;'"/>
                                            <xsl:with-param name="replacement" select="'\&amp;'"/>
                                          </xsl:call-template>
                                        </xsl:with-param>
                                        <xsl:with-param name="pattern" select="'#'"/>
                                        <xsl:with-param name="replacement" select="'\#'"/>
                                      </xsl:call-template>
                                    </xsl:with-param>
                                    <xsl:with-param name="pattern" select="'$'"/>
                                    <xsl:with-param name="replacement" select="'\$'"/>
                                  </xsl:call-template>
                                </xsl:with-param>
                                <xsl:with-param name="pattern" select="'%'"/>
                                <xsl:with-param name="replacement" select="'\%'"/>
                              </xsl:call-template>
                            </xsl:with-param>
                            <xsl:with-param name="pattern" select="'^'"/>
                            <xsl:with-param name="replacement" select="'\^{}'"/>
                          </xsl:call-template>
                        </xsl:with-param>
                        <xsl:with-param name="pattern" select="'{'"/>
                        <xsl:with-param name="replacement" select="'\{'"/>
                      </xsl:call-template>
                    </xsl:with-param>
                    <xsl:with-param name="pattern" select="'}'"/>
                    <xsl:with-param name="replacement" select="'\}'"/>
                  </xsl:call-template>
                </xsl:with-param>
                <xsl:with-param name="pattern" select="'~'"/>
                <xsl:with-param name="replacement" select="'\~{}'"/>
              </xsl:call-template>
            </xsl:with-param>
            <xsl:with-param name="pattern" select="'_'"/>
            <xsl:with-param name="replacement" select="'\_'"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="pattern" select="'&gt;'"/>
        <xsl:with-param name="replacement" select="'$&gt;$'"/>
      </xsl:call-template>
    </xsl:with-param>
    <xsl:with-param name="pattern" select="'&lt;'"/>
    <xsl:with-param name="replacement" select="'$&lt;$'"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>

