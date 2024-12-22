#ifndef CODE_GEN_WRITER_H
#define CODE_GEN_WRITER_H

#include <string>
#include <map>
#include <algorithm>

#include <QSet>
#include <QList>

#include "CodeWriter/Unit.h"
#include "Function.h"
#include "Type.h"
#include "CwMap.h"
#include "vector.h"

namespace cw
{

struct Unit;
struct Arrays;
class WriterHelper;

class Writer
{
public:
    void generate(Unit* unit);
    void setBannerAttribute(bool banner);
    void setStubAttribute(bool stub);

private:
    static QString toQString(Access access);
    static QString constQualifier(bool value);
    static QString initVal(const QString& value);
    static QString constexprQualifier(bool value);
    static QString referenceQualifier(bool value);
    static QString staticQualifier(bool value);
    static QString externQualifier(const Variable value);
    static QString virtualQualifier(bool value);

    QString  enumName(const QString& name) const;

    static
    QString getFunctionSignature(const Function& func);
    QString removeFileBanner(const QString& input_content, const QStringList& banner_lines);
    QString removeOnlyFileBanner(const Unit* unit, const QString& fileText);

    void writeFunctionImpl(const Function& func);
    void writeLangFunctionImpl(const Function& func);

    void writeFunctionHeader(const Function& func);

    WriterHelper* mH = nullptr;   // Header file
    WriterHelper* mC = nullptr;   // Implementation file

    QString macro = "#ifndef ";
    QString macro2 = "#define ";
    QString macro3 = "#endif";
    QString Directives = "#include ";
    QString ext_str = "extern";
    QString test;
    bool mBanner = false;
    bool mStub   = false;
    QVector<QString> mPrivateMemberVariables;
    QVector<QString> mPublicMembervariables;
    QVector<QString> mProtectedMemberVariables;

    bool writeHeader(const Unit* unit);
    bool writeImpl(const Unit* unit);

    QString toHeaderGaurdMacro(const QString& str) const;
    QString toUpper(const QString& str) const;

    void addEnum(const QString& name,
                 const QVector<QPair<QString, QString>>& values,
                 bool strongly_typed = true,
                 QString enum_type = QString());

    void addStructures(const Structures& struc);

    static
    QString writeClassDeclaration(const Type& clas);
    void writeClassImpl(const Type& clas);
    void writeClassHeader(const Type& clas);
    void writeArrayImpl(const Arrays& arrays, WriterHelper* writer);
    void writeMapImpl(const Map &maps, WriterHelper* writer);
    void writeVectorImpl(const Vector &vector, WriterHelper* writer);
    bool filesCompare(const QString& file_data_before, const QString& file_data_after);
    void handleVariableAccess(const QVector<Variable> &variables);

    template <typename T>
    QList<T> sorted(const QSet<T>& set)
    {
        auto list = set.values();

        std::stable_sort(list.begin(), list.end());

        return list;
    }

    template <typename T>
    QVector<T> sorted(const QVector<T>& vector)
    {
        auto list = vector;

        std::stable_sort(list.begin(), list.end());

        return list;
    }
};

} // namespace cw

#endif // CODE_GEN_WRITER_H
