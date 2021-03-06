/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestBrowser.h"

#include "TestGlobal.h"
#include "browser/BrowserSettings.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "sodium/crypto_box.h"

#include <QString>

QTEST_GUILESS_MAIN(TestBrowser)

const QString PUBLICKEY = "UIIPObeoya1G8g1M5omgyoPR/j1mR1HlYHu0wHCgMhA=";
const QString SECRETKEY = "B8ei4ZjQJkWzZU2SK/tBsrYRwp+6ztEMf5GFQV+i0yI=";
const QString SERVERPUBLICKEY = "lKnbLhrVCOqzEjuNoUz1xj9EZlz8xeO4miZBvLrUPVQ=";
const QString SERVERSECRETKEY = "tbPQcghxfOgbmsnEqG2qMIj1W2+nh+lOJcNsHncaz1Q=";
const QString NONCE = "zBKdvTjL5bgWaKMCTut/8soM/uoMrFoZ";
const QString CLIENTID = "testClient";

void TestBrowser::initTestCase()
{
    QVERIFY(Crypto::init());
    m_browserService = browserService();
    browserSettings()->setBestMatchOnly(false);
}

void TestBrowser::init()
{
    m_browserAction.reset(new BrowserAction());
}

/**
 * Tests for BrowserAction
 */

void TestBrowser::testChangePublicKeys()
{
    QJsonObject json;
    json["action"] = "change-public-keys";
    json["publicKey"] = PUBLICKEY;
    json["nonce"] = NONCE;

    auto response = m_browserAction->processClientMessage(json);
    QCOMPARE(response["action"].toString(), QString("change-public-keys"));
    QCOMPARE(response["publicKey"].toString() == PUBLICKEY, false);
    QCOMPARE(response["success"].toString(), TRUE_STR);
}

void TestBrowser::testEncryptMessage()
{
    QJsonObject message;
    message["action"] = "test-action";

    m_browserAction->m_publicKey = SERVERPUBLICKEY;
    m_browserAction->m_secretKey = SERVERSECRETKEY;
    m_browserAction->m_clientPublicKey = PUBLICKEY;
    auto encrypted = m_browserAction->encryptMessage(message, NONCE);

    QCOMPARE(encrypted, QString("+zjtntnk4rGWSl/Ph7Vqip/swvgeupk4lNgHEm2OO3ujNr0OMz6eQtGwjtsj+/rP"));
}

void TestBrowser::testDecryptMessage()
{
    QString message = "+zjtntnk4rGWSl/Ph7Vqip/swvgeupk4lNgHEm2OO3ujNr0OMz6eQtGwjtsj+/rP";
    m_browserAction->m_publicKey = SERVERPUBLICKEY;
    m_browserAction->m_secretKey = SERVERSECRETKEY;
    m_browserAction->m_clientPublicKey = PUBLICKEY;
    auto decrypted = m_browserAction->decryptMessage(message, NONCE);

    QCOMPARE(decrypted["action"].toString(), QString("test-action"));
}

void TestBrowser::testGetBase64FromKey()
{
    unsigned char pk[crypto_box_PUBLICKEYBYTES];

    for (unsigned int i = 0; i < crypto_box_PUBLICKEYBYTES; ++i) {
        pk[i] = i;
    }

    auto response = m_browserAction->getBase64FromKey(pk, crypto_box_PUBLICKEYBYTES);
    QCOMPARE(response, QString("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8="));
}

void TestBrowser::testIncrementNonce()
{
    auto result = m_browserAction->incrementNonce(NONCE);
    QCOMPARE(result, QString("zRKdvTjL5bgWaKMCTut/8soM/uoMrFoZ"));
}

/**
 * Tests for BrowserService
 */
void TestBrowser::testBaseDomain()
{
    QString url1 = "https://another.example.co.uk";
    QString url2 = "https://www.example.com";
    QString url3 = "http://test.net";
    QString url4 = "http://so.many.subdomains.co.jp";

    QString res1 = m_browserService->baseDomain(url1);
    QString res2 = m_browserService->baseDomain(url2);
    QString res3 = m_browserService->baseDomain(url3);
    QString res4 = m_browserService->baseDomain(url4);

    QCOMPARE(res1, QString("example.co.uk"));
    QCOMPARE(res2, QString("example.com"));
    QCOMPARE(res3, QString("test.net"));
    QCOMPARE(res4, QString("subdomains.co.jp"));
}

void TestBrowser::testSortPriority()
{
    QString host = "github.com";
    QString submitUrl = "https://github.com/session";
    QString baseSubmitUrl = "https://github.com";
    QString fullUrl = "https://github.com/login";

    QScopedPointer<Entry> entry1(new Entry());
    QScopedPointer<Entry> entry2(new Entry());
    QScopedPointer<Entry> entry3(new Entry());
    QScopedPointer<Entry> entry4(new Entry());
    QScopedPointer<Entry> entry5(new Entry());
    QScopedPointer<Entry> entry6(new Entry());
    QScopedPointer<Entry> entry7(new Entry());
    QScopedPointer<Entry> entry8(new Entry());
    QScopedPointer<Entry> entry9(new Entry());
    QScopedPointer<Entry> entry10(new Entry());
    QScopedPointer<Entry> entry11(new Entry());

    entry1->setUrl("https://github.com/login");
    entry2->setUrl("https://github.com/login");
    entry3->setUrl("https://github.com/");
    entry4->setUrl("github.com/login");
    entry5->setUrl("http://github.com");
    entry6->setUrl("http://github.com/login");
    entry7->setUrl("github.com");
    entry8->setUrl("github.com/login");
    entry9->setUrl("https://github"); // Invalid URL
    entry10->setUrl("github.com");
    entry11->setUrl("https://github.com/login"); // Exact match

    // The extension uses the submitUrl as default for comparison
    auto res1 = m_browserService->sortPriority(entry1.data(), host, "https://github.com/login", baseSubmitUrl, fullUrl);
    auto res2 = m_browserService->sortPriority(entry2.data(), host, submitUrl, baseSubmitUrl, baseSubmitUrl);
    auto res3 = m_browserService->sortPriority(entry3.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res4 = m_browserService->sortPriority(entry4.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res5 = m_browserService->sortPriority(entry5.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res6 = m_browserService->sortPriority(entry6.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res7 = m_browserService->sortPriority(entry7.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res8 = m_browserService->sortPriority(entry8.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res9 = m_browserService->sortPriority(entry9.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res10 = m_browserService->sortPriority(entry10.data(), host, submitUrl, baseSubmitUrl, fullUrl);
    auto res11 = m_browserService->sortPriority(entry11.data(), host, submitUrl, baseSubmitUrl, fullUrl);

    QCOMPARE(res1, 100);
    QCOMPARE(res2, 40);
    QCOMPARE(res3, 90);
    QCOMPARE(res4, 0);
    QCOMPARE(res5, 0);
    QCOMPARE(res6, 0);
    QCOMPARE(res7, 0);
    QCOMPARE(res8, 0);
    QCOMPARE(res9, 0);
    QCOMPARE(res10, 0);
    QCOMPARE(res11, 100);
}

void TestBrowser::testSearchEntries()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    QStringList urls = {"https://github.com/login_page",
                        "https://github.com/login",
                        "https://github.com/",
                        "github.com/login",
                        "http://github.com",
                        "http://github.com/login",
                        "github.com",
                        "github.com/login",
                        "https://github", // Invalid URL
                        "github.com"};

    createEntries(urls, root);

    browserSettings()->setMatchUrlScheme(false);
    auto result =
        m_browserService->searchEntries(db, "https://github.com", "https://github.com/session"); // db, url, submitUrl

    QCOMPARE(result.length(), 9);
    QCOMPARE(result[0]->url(), QString("https://github.com/login_page"));
    QCOMPARE(result[1]->url(), QString("https://github.com/login"));
    QCOMPARE(result[2]->url(), QString("https://github.com/"));
    QCOMPARE(result[3]->url(), QString("github.com/login"));
    QCOMPARE(result[4]->url(), QString("http://github.com"));
    QCOMPARE(result[5]->url(), QString("http://github.com/login"));

    // With matching there should be only 3 results + 4 without a scheme
    browserSettings()->setMatchUrlScheme(true);
    result = m_browserService->searchEntries(db, "https://github.com", "https://github.com/session");
    QCOMPARE(result.length(), 7);
    QCOMPARE(result[0]->url(), QString("https://github.com/login_page"));
    QCOMPARE(result[1]->url(), QString("https://github.com/login"));
    QCOMPARE(result[2]->url(), QString("https://github.com/"));
    QCOMPARE(result[3]->url(), QString("github.com/login"));
}

void TestBrowser::testSearchEntriesByUUID()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    /* The URLs don't really matter for this test, we just need some entries */
    QStringList urls = {"https://github.com/login_page",
                        "https://github.com/login",
                        "https://github.com/",
                        "github.com/login",
                        "http://github.com",
                        "http://github.com/login",
                        "github.com",
                        "github.com/login",
                        "https://github",
                        "github.com",
                        "",
                        "not an URL"};
    auto entries = createEntries(urls, root);

    for (Entry* entry : entries) {
        QString testUrl = "keepassxc://by-uuid/" + entry->uuidToHex();
        /* Look for an entry with that UUID. First using handleEntry, then through the search */
        QCOMPARE(m_browserService->handleEntry(entry, testUrl, ""), true);
        auto result = m_browserService->searchEntries(db, testUrl, "");
        QCOMPARE(result.length(), 1);
        QCOMPARE(result[0], entry);
    }

    /* Test for entries that don't exist */
    QStringList uuids = {"00000000000000000000000000000000",
                         "00000000000000000000000000000001",
                         "00000000000000000000000000000002/",
                         "invalid uuid",
                         "000000000000000000000000000000000000000"
                         "00000000000000000000000"};

    for (QString uuid : uuids) {
        QString testUrl = "keepassxc://by-uuid/" + uuid;

        for (Entry* entry : entries) {
            QCOMPARE(m_browserService->handleEntry(entry, testUrl, ""), false);
        }

        auto result = m_browserService->searchEntries(db, testUrl, "");
        QCOMPARE(result.length(), 0);
    }
}

void TestBrowser::testSearchEntriesWithPort()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    QStringList urls = {"http://127.0.0.1:443", "http://127.0.0.1:80"};

    createEntries(urls, root);

    auto result = m_browserService->searchEntries(db, "http://127.0.0.1:443", "http://127.0.0.1");
    QCOMPARE(result.length(), 1);
    QCOMPARE(result[0]->url(), QString("http://127.0.0.1:443"));
}

void TestBrowser::testSearchEntriesWithAdditionalURLs()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    QStringList urls = {"https://github.com/", "https://www.example.com", "http://domain.com"};

    auto entries = createEntries(urls, root);

    // Add an additional URL to the first entry
    entries.first()->attributes()->set(BrowserService::ADDITIONAL_URL, "https://keepassxc.org");

    auto result = m_browserService->searchEntries(db, "https://github.com", "https://github.com/session");
    QCOMPARE(result.length(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com/"));

    // Search the additional URL. It should return the same entry
    auto additionalResult = m_browserService->searchEntries(db, "https://keepassxc.org", "https://keepassxc.org");
    QCOMPARE(additionalResult.length(), 1);
    QCOMPARE(additionalResult[0]->url(), QString("https://github.com/"));
}

void TestBrowser::testInvalidEntries()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();
    const QString url("https://github.com");
    const QString submitUrl("https://github.com/session");

    QStringList urls = {
        "https://github.com/login",
        "https:///github.com/", // Extra '/'
        "http://github.com/**//*",
        "http://*.github.com/login",
        "//github.com", // fromUserInput() corrects this one.
        "github.com/{}<>",
        "http:/example.com",
    };

    createEntries(urls, root);

    browserSettings()->setMatchUrlScheme(true);
    auto result = m_browserService->searchEntries(db, "https://github.com", "https://github.com/session");
    QCOMPARE(result.length(), 2);
    QCOMPARE(result[0]->url(), QString("https://github.com/login"));
    QCOMPARE(result[1]->url(), QString("//github.com"));

    // Test the URL's directly
    QCOMPARE(m_browserService->handleURL(urls[0], url, submitUrl), true);
    QCOMPARE(m_browserService->handleURL(urls[1], url, submitUrl), false);
    QCOMPARE(m_browserService->handleURL(urls[2], url, submitUrl), false);
    QCOMPARE(m_browserService->handleURL(urls[3], url, submitUrl), false);
    QCOMPARE(m_browserService->handleURL(urls[4], url, submitUrl), true);
    QCOMPARE(m_browserService->handleURL(urls[5], url, submitUrl), false);
}

void TestBrowser::testSubdomainsAndPaths()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    QStringList urls = {
        "https://www.github.com/login/page.xml",
        "https://login.github.com/",
        "https://github.com",
        "http://www.github.com",
        "http://login.github.com/pathtonowhere",
        ".github.com", // Invalid URL
        "www.github.com/",
        "https://github", // Invalid URL
        "https://hub.com" // Should not return
    };

    createEntries(urls, root);

    browserSettings()->setMatchUrlScheme(false);
    auto result = m_browserService->searchEntries(db, "https://github.com", "https://github.com/session");
    QCOMPARE(result.length(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com"));

    // With www subdomain
    result = m_browserService->searchEntries(db, "https://www.github.com", "https://www.github.com/session");
    QCOMPARE(result.length(), 4);
    QCOMPARE(result[0]->url(), QString("https://www.github.com/login/page.xml"));
    QCOMPARE(result[1]->url(), QString("https://github.com")); // Accepts any subdomain
    QCOMPARE(result[2]->url(), QString("http://www.github.com"));
    QCOMPARE(result[3]->url(), QString("www.github.com/"));

    // With scheme matching there should be only 1 result
    browserSettings()->setMatchUrlScheme(true);
    result = m_browserService->searchEntries(db, "https://github.com", "https://github.com/session");
    QCOMPARE(result.length(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com"));

    // Test site with subdomain in the site URL
    QStringList entryURLs = {
        "https://accounts.example.com",
        "https://accounts.example.com/path",
        "https://subdomain.example.com/",
        "https://another.accounts.example.com/",
        "https://another.subdomain.example.com/",
        "https://example.com/",
        "https://example" // Invalid URL
    };

    createEntries(entryURLs, root);

    result = m_browserService->searchEntries(db, "https://accounts.example.com", "https://accounts.example.com");
    QCOMPARE(result.length(), 3);
    QCOMPARE(result[0]->url(), QString("https://accounts.example.com"));
    QCOMPARE(result[1]->url(), QString("https://accounts.example.com/path"));
    QCOMPARE(result[2]->url(), QString("https://example.com/")); // Accepts any subdomain

    result = m_browserService->searchEntries(
        db, "https://another.accounts.example.com", "https://another.accounts.example.com");
    QCOMPARE(result.length(), 4);
    QCOMPARE(result[0]->url(),
             QString("https://accounts.example.com")); // Accepts any subdomain under accounts.example.com
    QCOMPARE(result[1]->url(), QString("https://accounts.example.com/path"));
    QCOMPARE(result[2]->url(), QString("https://another.accounts.example.com/"));
    QCOMPARE(result[3]->url(), QString("https://example.com/")); // Accepts one or more subdomains

    // Test local files. It should be a direct match.
    QStringList localFiles = {"file:///Users/testUser/tests/test.html"};

    createEntries(localFiles, root);

    // With local files, url is always set to the file scheme + ://. Submit URL holds the actual URL.
    result = m_browserService->searchEntries(db, "file://", "file:///Users/testUser/tests/test.html");
    QCOMPARE(result.length(), 1);
}

void TestBrowser::testSortEntries()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    QStringList urls = {"https://github.com/login_page",
                        "https://github.com/login",
                        "https://github.com/",
                        "github.com/login",
                        "http://github.com",
                        "http://github.com/login",
                        "github.com",
                        "github.com/login",
                        "https://github", // Invalid URL
                        "github.com"};

    auto entries = createEntries(urls, root);

    browserSettings()->setBestMatchOnly(false);
    auto result = m_browserService->sortEntries(
        entries, "github.com", "https://github.com/session", "https://github.com"); // entries, host, submitUrl
    QCOMPARE(result.size(), 10);
    QCOMPARE(result[0]->username(), QString("User 2"));
    QCOMPARE(result[0]->url(), QString("https://github.com/"));
    QCOMPARE(result[1]->username(), QString("User 0"));
    QCOMPARE(result[1]->url(), QString("https://github.com/login_page"));
    QCOMPARE(result[2]->username(), QString("User 1"));
    QCOMPARE(result[2]->url(), QString("https://github.com/login"));
    QCOMPARE(result[3]->username(), QString("User 3"));
    QCOMPARE(result[3]->url(), QString("github.com/login"));

    // Test with a perfect match. That should be first in the list.
    result = m_browserService->sortEntries(
        entries, "github.com", "https://github.com/session", "https://github.com/login_page");
    QCOMPARE(result.size(), 10);
    QCOMPARE(result[0]->username(), QString("User 0"));
    QCOMPARE(result[0]->url(), QString("https://github.com/login_page"));
    QCOMPARE(result[1]->username(), QString("User 2"));
    QCOMPARE(result[1]->url(), QString("https://github.com/"));
}

QList<Entry*> TestBrowser::createEntries(QStringList& urls, Group* root) const
{
    QList<Entry*> entries;
    for (int i = 0; i < urls.length(); ++i) {
        auto entry = new Entry();
        entry->setGroup(root);
        entry->beginUpdate();
        entry->setUrl(urls[i]);
        entry->setUsername(QString("User %1").arg(i));
        entry->setUuid(QUuid::createUuid());
        entry->endUpdate();
        entries.push_back(entry);
    }

    return entries;
}
void TestBrowser::testValidURLs()
{
    QHash<QString, bool> urls;
    urls["https://github.com/login"] = true;
    urls["https:///github.com/"] = false;
    urls["http://github.com/**//*"] = false;
    urls["http://*.github.com/login"] = false;
    urls["//github.com"] = true;
    urls["github.com/{}<>"] = false;
    urls["http:/example.com"] = false;
    urls["cmd://C:/Toolchains/msys2/usr/bin/mintty \"ssh jon@192.168.0.1:22\""] = true;
    urls["file:///Users/testUser/Code/test.html"] = true;

    QHashIterator<QString, bool> i(urls);
    while (i.hasNext()) {
        i.next();
        QCOMPARE(Tools::checkUrlValid(i.key()), i.value());
    }
}

void TestBrowser::testBestMatchingCredentials()
{
    auto db = QSharedPointer<Database>::create();
    auto* root = db->rootGroup();

    // Test with simple URL entries
    QStringList urls = {"https://github.com/loginpage", "https://github.com/justsomepage", "https://github.com/"};

    auto entries = createEntries(urls, root);

    browserSettings()->setBestMatchOnly(true);

    auto result = m_browserService->searchEntries(db, "https://github.com/loginpage", "https://github.com/loginpage");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com/loginpage"));

    result = m_browserService->searchEntries(db, "https://github.com/justsomepage", "https://github.com/justsomepage");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com/justsomepage"));

    result = m_browserService->searchEntries(db, "https://github.com/", "https://github.com/");
    m_browserService->sortEntries(entries, "github.com", "https://github.com/", "https://github.com/");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com/"));

    browserSettings()->setBestMatchOnly(false);
    result = m_browserService->searchEntries(db, "https://github.com/loginpage", "https://github.com/loginpage");
    QCOMPARE(result.size(), 3);
    QCOMPARE(result[0]->url(), QString("https://github.com/loginpage"));

    // Test with subdomains
    QStringList subdomainsUrls = {"https://sub.github.com/loginpage",
                                  "https://sub.github.com/justsomepage",
                                  "https://bus.github.com/justsomepage"};

    entries = createEntries(subdomainsUrls, root);

    browserSettings()->setBestMatchOnly(true);

    result = m_browserService->searchEntries(
        db, "https://sub.github.com/justsomepage", "https://sub.github.com/justsomepage");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://sub.github.com/justsomepage"));

    result = m_browserService->searchEntries(db, "https://github.com/justsomepage", "https://github.com/justsomepage");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://github.com/justsomepage"));

    result = m_browserService->searchEntries(db,
                                             "https://sub.github.com/justsomepage?wehavesomeextra=here",
                                             "https://sub.github.com/justsomepage?wehavesomeextra=here");
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0]->url(), QString("https://sub.github.com/justsomepage"));
}
